from __future__ import annotations

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import vgm_creator_part_matcher as matcher  # noqa: E402


def part(channel: int, intervals: dict[str, int], bigrams: dict[str, int]) -> dict[str, object]:
    return {
        "channel": channel,
        "key_ons": 8,
        "interval_histogram_semitones": intervals,
        "interval_bigram_histogram": bigrams,
    }


def track(*parts: dict[str, object]) -> dict[str, object]:
    return {"parts": list(parts)}


def test_part_similarity_is_untuned_equal_weight_mean() -> None:
    lhs = part(0, {"2": 3, "-2": 1}, {"2,-2": 4})
    rhs = part(4, {"2": 3, "-2": 1}, {"7,7": 4})

    # Perfect interval match plus orthogonal bigram match = exactly 0.5.
    assert matcher.part_similarity(lhs, rhs) == 0.5


def test_missing_bigram_view_contributes_zero_not_weight_shift() -> None:
    lhs = part(0, {"2": 3}, {})
    rhs = part(1, {"2": 5}, {})

    assert matcher.part_similarity(lhs, rhs) == 0.5


def test_channel_numbers_are_not_cross_track_correspondence() -> None:
    rising = part(0, {"2": 5}, {"2,2": 4})
    falling = part(1, {"-2": 5}, {"-2,-2": 4})

    # Same two musical parts, deliberately moved to different physical channels.
    moved_falling = part(4, {"-2": 5}, {"-2,-2": 4})
    moved_rising = part(5, {"2": 5}, {"2,2": 4})

    assert matcher.track_similarity(
        track(rising, falling),
        track(moved_falling, moved_rising),
    ) == 1.0


def test_assignment_is_one_to_one_and_globally_maximized() -> None:
    lhs = track(
        part(0, {"1": 8}, {"1,1": 7}),
        part(1, {"5": 8}, {"5,5": 7}),
    )
    rhs = track(
        part(4, {"5": 8}, {"5,5": 7}),
        part(5, {"1": 8}, {"1,1": 7}),
    )

    detail = matcher.track_similarity(lhs, rhs, include_assignment=True)
    assert detail["similarity"] == 1.0
    assert detail["matched_parts"] == 2
    assert {(row["left_channel"], row["right_channel"]) for row in detail["assignment"]} == {
        (0, 5),
        (1, 4),
    }


def test_unmatched_extra_part_is_ignored_exactly_as_preregistered() -> None:
    core = part(0, {"3": 5}, {"3,3": 4})
    same = part(2, {"3": 5}, {"3,3": 4})
    extra = part(5, {"-9": 5}, {"-9,-9": 4})

    assert matcher.track_similarity(track(core), track(same, extra)) == 1.0
    assert matcher.track_similarity(track(same, extra), track(core)) == 1.0


def test_empty_track_fails_closed_to_zero_similarity() -> None:
    assert matcher.track_similarity(track(), track(part(0, {"1": 1}, {}))) == 0.0


def test_audit_contract_remains_creator_blind() -> None:
    assert "composer" in matcher.LABEL_POLICY.lower()
    assert "metadata" in matcher.LABEL_POLICY.lower()
    assert "channel number" in matcher.LABEL_POLICY.lower()
