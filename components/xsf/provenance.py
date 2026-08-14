"""Byte-overlay provenance shared as mechanism, not machine semantics."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class ByteContribution:
    source_id: str
    source_offset: int
    target_start: int
    target_end: int
    stage_index: int
    role: str


class OverlayBuffer:
    def __init__(self, *, max_size: int) -> None:
        self._data = bytearray()
        self.max_size = max_size
        self.contributions: list[ByteContribution] = []

    def overlay(
        self,
        *,
        start: int,
        payload: bytes,
        source_id: str,
        source_offset: int,
        stage_index: int,
        role: str,
    ) -> None:
        if start < 0 or start + len(payload) > self.max_size:
            raise ValueError(f"overlay exceeds bounded effective object: {source_id}")
        end = start + len(payload)
        if len(self._data) < end:
            self._data.extend(b"\x00" * (end - len(self._data)))
        self._data[start:end] = payload
        if payload:
            self.contributions.append(
                ByteContribution(
                    source_id=source_id,
                    source_offset=source_offset,
                    target_start=start,
                    target_end=end,
                    stage_index=stage_index,
                    role=role,
                )
            )

    @property
    def data(self) -> bytes:
        return bytes(self._data)

    def provenance_at(self, offset: int) -> ByteContribution | None:
        for contribution in reversed(self.contributions):
            if contribution.target_start <= offset < contribution.target_end:
                return contribution
        return None
