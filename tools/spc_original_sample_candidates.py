#!/usr/bin/env python3
"""Rank possible upstream sources for a decoded SNES/BRR sample.

This is a retrieval tool, not a provenance oracle. It deliberately uses only
content-preserving operations that are plausible in an old sample-preparation
chain: channel folding, DC removal, gain normalization, coarse time resampling,
window selection, and multiscale smoothing. A high score creates a candidate for
lineage verification; it never grants automatic Enhanced replacement by itself.

The pure-Python implementation is intentionally dependency-light so corpus
machines can run a first pass without a DSP stack. WAV PCM is the first accepted
container. Richer corpus decoders may feed the same array-level matcher later.
"""

from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
import json
import math
from pathlib import Path
import struct
import wave
from typing import Iterable, Sequence


@dataclass(frozen=True)
class PcmSignal:
    samples: tuple[float, ...]
    sample_rate: int


@dataclass(frozen=True)
class CandidateMatch:
    path: str
    score: float
    waveform_correlation: float
    derivative_correlation: float
    query_frames: int
    candidate_frames: int
    candidate_window_start: int
    candidate_window_frames: int
    candidate_sample_rate: int


def _decode_integer_sample(raw: bytes, width: int) -> int:
    if width == 1:
        return raw[0] - 128
    if width == 2:
        return struct.unpack("<h", raw)[0]
    if width == 3:
        value = raw[0] | (raw[1] << 8) | (raw[2] << 16)
        if value & 0x800000:
            value -= 1 << 24
        return value
    if width == 4:
        return struct.unpack("<i", raw)[0]
    raise ValueError(f"unsupported PCM sample width: {width}")


def read_pcm_wav(path: Path) -> PcmSignal:
    with wave.open(str(path), "rb") as handle:
        channels = handle.getnchannels()
        width = handle.getsampwidth()
        rate = handle.getframerate()
        frames = handle.getnframes()
        if channels <= 0 or width not in (1, 2, 3, 4) or rate <= 0 or frames <= 0:
            raise ValueError(f"unsupported or empty WAV: {path}")
        raw = handle.readframes(frames)

    stride = channels * width
    if len(raw) < frames * stride:
        raise ValueError(f"truncated WAV data: {path}")

    mono: list[float] = []
    for frame in range(frames):
        base = frame * stride
        total = 0.0
        for channel in range(channels):
            start = base + channel * width
            total += float(_decode_integer_sample(raw[start : start + width], width))
        mono.append(total / float(channels))
    return PcmSignal(tuple(mono), rate)


def remove_dc(samples: Sequence[float]) -> list[float]:
    if not samples:
        return []
    mean = math.fsum(samples) / len(samples)
    return [float(value) - mean for value in samples]


def normalize_rms(samples: Sequence[float]) -> list[float]:
    centered = remove_dc(samples)
    if not centered:
        return []
    energy = math.fsum(value * value for value in centered) / len(centered)
    if not math.isfinite(energy) or energy <= 1.0e-20:
        return [0.0 for _ in centered]
    scale = 1.0 / math.sqrt(energy)
    return [value * scale for value in centered]


def resample_to_length(samples: Sequence[float], output_frames: int) -> list[float]:
    if output_frames <= 0 or not samples:
        return []
    if len(samples) == 1:
        return [float(samples[0])] * output_frames
    if output_frames == 1:
        return [float(samples[0])]

    source_last = len(samples) - 1
    output_last = output_frames - 1
    result: list[float] = []
    for index in range(output_frames):
        position = index * source_last / output_last
        left = int(math.floor(position))
        right = min(left + 1, source_last)
        fraction = position - left
        result.append(
            float(samples[left]) * (1.0 - fraction)
            + float(samples[right]) * fraction
        )
    return result


def smooth_box(samples: Sequence[float], radius: int) -> list[float]:
    if radius <= 0:
        return [float(value) for value in samples]
    if not samples:
        return []
    prefix = [0.0]
    running = 0.0
    for value in samples:
        running += float(value)
        prefix.append(running)
    output: list[float] = []
    for index in range(len(samples)):
        lo = max(0, index - radius)
        hi = min(len(samples), index + radius + 1)
        output.append((prefix[hi] - prefix[lo]) / (hi - lo))
    return output


def correlation(left: Sequence[float], right: Sequence[float]) -> float:
    if len(left) != len(right) or len(left) < 2:
        return 0.0
    a = normalize_rms(left)
    b = normalize_rms(right)
    if not a or not b or not any(abs(value) > 1.0e-12 for value in a):
        return 0.0
    if not any(abs(value) > 1.0e-12 for value in b):
        return 0.0
    value = math.fsum(x * y for x, y in zip(a, b)) / len(a)
    return max(-1.0, min(1.0, value))


def first_difference(samples: Sequence[float]) -> list[float]:
    return [float(samples[index]) - float(samples[index - 1]) for index in range(1, len(samples))]


def multiscale_similarity(
    query: Sequence[float],
    candidate: Sequence[float],
    comparison_frames: int = 256,
) -> tuple[float, float, float]:
    if len(query) < 4 or len(candidate) < 4 or comparison_frames < 16:
        return (0.0, 0.0, 0.0)

    q = resample_to_length(query, comparison_frames)
    c = resample_to_length(candidate, comparison_frames)

    correlations: list[float] = []
    for radius in (0, 1, 2, 4, 8):
        correlations.append(correlation(smooth_box(q, radius), smooth_box(c, radius)))
    waveform = math.fsum(correlations) / len(correlations)

    qd = first_difference(q)
    cd = first_difference(c)
    derivative = correlation(smooth_box(qd, 2), smooth_box(cd, 2))

    # Low-frequency/multiscale shape carries more weight than derivatives because
    # BRR quantization and source filtering attack local edges first.
    score = 0.78 * waveform + 0.22 * derivative
    return (score, waveform, derivative)


def candidate_window_lengths(query: PcmSignal, candidate: PcmSignal) -> list[int]:
    # The nominal duration is a useful center, but old game preparation may have
    # resampled, truncated, or extended a loop. Search a broad multiplicative
    # neighborhood without pretending one ratio is known beforehand.
    expected = len(query.samples) * candidate.sample_rate / query.sample_rate
    ratios = (0.50, 0.63, 0.79, 1.0, 1.26, 1.59, 2.0)
    lengths = {
        max(8, min(len(candidate.samples), int(round(expected * ratio))))
        for ratio in ratios
    }
    lengths.add(len(candidate.samples))
    return sorted(lengths)


def best_signal_match(
    query: PcmSignal,
    candidate: PcmSignal,
    comparison_frames: int = 256,
    max_windows_per_length: int = 96,
) -> tuple[float, float, float, int, int]:
    if not query.samples or not candidate.samples:
        return (0.0, 0.0, 0.0, 0, 0)

    best = (-2.0, 0.0, 0.0, 0, 0)
    total = len(candidate.samples)
    for window_frames in candidate_window_lengths(query, candidate):
        if window_frames <= 0 or window_frames > total:
            continue
        available = total - window_frames
        if available == 0:
            starts = [0]
        else:
            count = min(max_windows_per_length, available + 1)
            starts = sorted({
                int(round(index * available / max(1, count - 1)))
                for index in range(count)
            })

        for start in starts:
            window = candidate.samples[start : start + window_frames]
            score, waveform, derivative = multiscale_similarity(
                query.samples, window, comparison_frames
            )
            if score > best[0]:
                best = (score, waveform, derivative, start, window_frames)
    return best


def iter_wavs(root: Path) -> Iterable[Path]:
    if root.is_file():
        if root.suffix.lower() == ".wav":
            yield root
        return
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.suffix.lower() == ".wav":
            yield path


def rank_candidates(
    query_path: Path,
    candidate_root: Path,
    limit: int = 20,
    comparison_frames: int = 256,
) -> list[CandidateMatch]:
    query = read_pcm_wav(query_path)
    matches: list[CandidateMatch] = []
    for path in iter_wavs(candidate_root):
        if path.resolve() == query_path.resolve():
            continue
        try:
            candidate = read_pcm_wav(path)
        except (ValueError, wave.Error, EOFError):
            continue
        score, waveform, derivative, start, window_frames = best_signal_match(
            query, candidate, comparison_frames=comparison_frames
        )
        matches.append(
            CandidateMatch(
                path=str(path),
                score=score,
                waveform_correlation=waveform,
                derivative_correlation=derivative,
                query_frames=len(query.samples),
                candidate_frames=len(candidate.samples),
                candidate_window_start=start,
                candidate_window_frames=window_frames,
                candidate_sample_rate=candidate.sample_rate,
            )
        )

    matches.sort(key=lambda item: item.score, reverse=True)
    return matches[: max(0, limit)]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("query", type=Path, help="decoded game/BRR sample as PCM WAV")
    parser.add_argument("candidates", type=Path, help="WAV file or directory tree to search")
    parser.add_argument("--limit", type=int, default=20)
    parser.add_argument("--comparison-frames", type=int, default=256)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    matches = rank_candidates(
        args.query,
        args.candidates,
        limit=args.limit,
        comparison_frames=args.comparison_frames,
    )
    payload = {
        "schema": "spc-original-sample-candidates-001",
        "query": str(args.query),
        "candidate_root": str(args.candidates),
        "claim_boundary": (
            "ranking is candidate retrieval only; similarity is not exact lineage, "
            "historical provenance, or automatic Enhanced permission"
        ),
        "matches": [asdict(match) for match in matches],
    }
    text = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
    else:
        print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
