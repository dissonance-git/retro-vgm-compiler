import math
import unittest

from tools.spc_original_sample_candidates import (
    PcmSignal,
    best_signal_match,
    multiscale_similarity,
)


class OriginalSampleCandidateTests(unittest.TestCase):
    def test_resampled_gain_shifted_source_still_matches(self):
        source = [
            math.sin(index * 0.17)
            + 0.32 * math.sin(index * 0.071 + 0.2)
            for index in range(420)
        ]
        # Game-like descendant: select a window, resample its duration, apply
        # gain/DC, and add small deterministic quantization-like perturbation.
        upstream_window = source[70:330]
        game = []
        for index in range(173):
            position = index * (len(upstream_window) - 1) / 172.0
            left = int(math.floor(position))
            right = min(left + 1, len(upstream_window) - 1)
            fraction = position - left
            value = upstream_window[left] * (1.0 - fraction) + upstream_window[right] * fraction
            value = value * 0.42 + 0.12
            value += ((index % 7) - 3) * 0.0009
            game.append(value)

        score, waveform, derivative, start, frames = best_signal_match(
            PcmSignal(tuple(game), 32000),
            PcmSignal(tuple(source), 48000),
        )
        self.assertGreater(score, 0.93)
        self.assertGreater(waveform, 0.95)
        self.assertGreater(derivative, 0.80)
        self.assertGreater(frames, 0)
        self.assertGreaterEqual(start, 0)

    def test_unrelated_waveform_scores_lower(self):
        query = [math.sin(index * 0.14) for index in range(220)]
        related = [math.sin(index * 0.14) for index in range(260)]
        unrelated = [
            math.sin(index * 0.47) * math.sin(index * 0.031)
            for index in range(260)
        ]

        related_score = best_signal_match(
            PcmSignal(tuple(query), 32000),
            PcmSignal(tuple(related), 32000),
        )[0]
        unrelated_score = best_signal_match(
            PcmSignal(tuple(query), 32000),
            PcmSignal(tuple(unrelated), 32000),
        )[0]
        self.assertGreater(related_score, unrelated_score + 0.20)

    def test_similarity_is_polarity_sensitive(self):
        query = [math.sin(index * 0.13) for index in range(128)]
        inverted = [-value for value in query]
        score, waveform, derivative = multiscale_similarity(query, inverted)
        self.assertLess(score, -0.90)
        self.assertLess(waveform, -0.90)
        self.assertLess(derivative, -0.90)


if __name__ == "__main__":
    unittest.main()
