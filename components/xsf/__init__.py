"""Common xSF envelope and dependency semantics."""

from .envelope import (
    DependencyEdge,
    ResolvedXsf,
    XsfDependencyError,
    XsfError,
    XsfObject,
    parse_xsf,
    resolve_xsf,
)

__all__ = [
    "DependencyEdge",
    "ResolvedXsf",
    "XsfDependencyError",
    "XsfError",
    "XsfObject",
    "parse_xsf",
    "resolve_xsf",
]
