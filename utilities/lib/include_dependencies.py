# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Source-level C++ include dependency graph and analysis helpers."""

from __future__ import annotations

import json
import posixpath
import re
from collections import Counter, defaultdict, deque
from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from lib.error import UtilityError

SOURCE_SUFFIXES = frozenset({".hpp", ".tpp", ".cpp"})
INCLUDE_PATTERN = re.compile(r'^[ \t]*#[ \t]*include[ \t]*([<"])([^>"]+)[>"]', re.MULTILINE)


@dataclass(frozen=True)
class DirectInclude:
    """One direct include directive in a C++ source file."""

    line_number: int
    target: str
    system: bool


@dataclass(frozen=True)
class IncludeEdge:
    """One resolved include edge in the dependency graph."""

    source: str
    target: str
    line_number: int
    system: bool
    internal: bool


@dataclass(frozen=True)
class IncludeNode:
    """One internal library source node."""

    path: str
    role: str
    public: bool


@dataclass(frozen=True)
class PublicHeaderPolicy:
    """Rules matching the public-header selection used by update_includes."""

    excluded_directories: frozenset[str] = frozenset()
    excluded_headers: frozenset[Path] = frozenset()

    def is_public(self, path: Path) -> bool:
        """Test whether a source-root-relative path is a regular public header."""
        if path.suffix != ".hpp" or path.name in {"all.hpp", "fwd.hpp"}:
            return False
        if "_" in path.stem or path in self.excluded_headers:
            return False
        if any(part in self.excluded_directories for part in path.parts[:-1]):
            return False
        return len(path.parts) >= 2


@dataclass(frozen=True)
class DependencyRule:
    """A configured transitive required-dependency rule."""

    name: str
    target: str
    excluded_headers: frozenset[str] = frozenset()


@dataclass(frozen=True)
class RequiredDependencyAnalysis:
    """The affected headers and minimal insertion frontier for one rule."""

    target: str
    missing_headers: tuple[str, ...]
    insertion_points: tuple[str, ...]


def direct_includes(text: str) -> tuple[DirectInclude, ...]:
    """Extract direct include directives from source text."""
    result = []
    for match in INCLUDE_PATTERN.finditer(text):
        result.append(
            DirectInclude(
                line_number=text.count("\n", 0, match.start()) + 1,
                target=match.group(2),
                system=match.group(1) == "<",
            )
        )
    return tuple(result)


def infer_role(path: Path) -> str:
    """Infer the graph role of a library source file."""
    if path.name == "all.hpp":
        return "generated"
    if path.name.endswith("_fwd.hpp") or path.name == "fwd.hpp":
        return "forward"
    if path.suffix == ".tpp":
        return "template"
    if path.suffix == ".cpp":
        return "source"
    return "header"


def resolve_local_include(source: Path, include: DirectInclude) -> Path | None:
    """Resolve a quoted include relative to its containing file."""
    if include.system:
        return None
    return (source.parent / include.target).resolve()


def discover_source_paths(source_directory: Path) -> list[Path]:
    """Discover every C++ library source file in deterministic order."""
    return sorted(
        (path for path in source_directory.rglob("*") if path.is_file() and path.suffix in SOURCE_SUFFIXES),
        key=lambda item: item.as_posix().casefold(),
    )


class IncludeGraph:
    """A portable graph of all explicit library include directives."""

    SCHEMA_VERSION = 1

    def __init__(
        self,
        source_directory: Path,
        nodes: Iterable[IncludeNode],
        edges: Iterable[IncludeEdge],
    ) -> None:
        self.source_directory = source_directory.resolve()
        self.nodes = {node.path: node for node in nodes}
        self.edges = tuple(
            sorted(edges, key=lambda edge: (edge.source.casefold(), edge.line_number, edge.target.casefold()))
        )
        adjacency: dict[str, list[IncludeEdge]] = defaultdict(list)
        reverse: dict[str, list[IncludeEdge]] = defaultdict(list)
        for edge in self.edges:
            adjacency[edge.source].append(edge)
            if edge.internal:
                reverse[edge.target].append(edge)
        self._adjacency = {
            path: tuple(sorted(path_edges, key=lambda edge: (edge.target.casefold(), edge.line_number)))
            for path, path_edges in adjacency.items()
        }
        self._reverse = {
            path: tuple(sorted(path_edges, key=lambda edge: (edge.source.casefold(), edge.line_number)))
            for path, path_edges in reverse.items()
        }

    @classmethod
    def from_source_directory(
        cls,
        source_directory: Path,
        public_policy: PublicHeaderPolicy | None = None,
    ) -> IncludeGraph:
        """Build an include graph from a library source directory."""
        source_directory = source_directory.resolve()
        if not source_directory.is_dir():
            raise UtilityError(f"Library source directory not found: {source_directory}")
        policy = public_policy or PublicHeaderPolicy()
        paths = discover_source_paths(source_directory)
        path_set = {path.resolve() for path in paths}
        nodes = []
        edges = []
        unresolved = []
        for path in paths:
            relative = path.relative_to(source_directory).as_posix()
            nodes.append(
                IncludeNode(
                    path=relative,
                    role=infer_role(path),
                    public=policy.is_public(Path(relative)),
                )
            )
            text = path.read_text(encoding="utf-8")
            for include in direct_includes(text):
                if include.system:
                    edges.append(
                        IncludeEdge(
                            source=relative,
                            target=include.target,
                            line_number=include.line_number,
                            system=True,
                            internal=False,
                        )
                    )
                    continue
                target_path = resolve_local_include(path, include)
                if target_path not in path_set:
                    unresolved.append(f"{relative}:{include.line_number}: {include.target}")
                    continue
                edges.append(
                    IncludeEdge(
                        source=relative,
                        target=target_path.relative_to(source_directory).as_posix(),
                        line_number=include.line_number,
                        system=False,
                        internal=True,
                    )
                )
        if unresolved:
            raise UtilityError("Unresolved quoted includes:\n" + "\n".join(f"- {entry}" for entry in unresolved))
        return cls(source_directory, nodes, edges)

    @classmethod
    def from_project(
        cls,
        project_directory: Path,
        public_policy: PublicHeaderPolicy | None = None,
    ) -> IncludeGraph:
        """Build the graph for the standard Erbsland Core source directory."""
        return cls.from_source_directory(project_directory / "src" / "erbsland", public_policy)

    @property
    def internal_edges(self) -> tuple[IncludeEdge, ...]:
        """Access all internal graph edges."""
        return tuple(edge for edge in self.edges if edge.internal)

    @property
    def external_headers(self) -> tuple[str, ...]:
        """Access all unique external/system include names."""
        return tuple(sorted({edge.target for edge in self.edges if not edge.internal}, key=str.casefold))

    def resolve_identifier(self, identifier: str) -> str:
        """Resolve a full or unambiguous suffix path to a canonical graph path."""
        candidate = identifier.strip().replace("\\", "/").removeprefix("./")
        candidate = candidate.removeprefix("src/erbsland/")
        if candidate in self.nodes:
            return candidate
        matches = sorted(
            (path for path in self.nodes if path.endswith(f"/{candidate}")),
            key=str.casefold,
        )
        if not matches:
            raise UtilityError(f"Library source path not found: {identifier}")
        if len(matches) > 1:
            raise UtilityError(
                f"Ambiguous library source path: {identifier}\n" + "\n".join(f"- {path}" for path in matches)
            )
        return matches[0]

    def dependency_edges(self, path: str, reverse: bool = False) -> tuple[IncludeEdge, ...]:
        """Access direct dependency or dependent edges for one internal node."""
        canonical = self.resolve_identifier(path)
        return (self._reverse if reverse else self._adjacency).get(canonical, ())

    def adjacent_internal_paths(self, path: str, reverse: bool = False) -> tuple[str, ...]:
        """Access unique adjacent internal paths in deterministic order."""
        edges = self.dependency_edges(path, reverse)
        paths = {edge.source if reverse else edge.target for edge in edges if edge.internal}
        return tuple(sorted(paths, key=str.casefold))

    def reachable(self, path: str, reverse: bool = False) -> set[str]:
        """Return every internal node reachable from the selected node."""
        canonical = self.resolve_identifier(path)
        result: set[str] = set()
        pending = list(self.adjacent_internal_paths(canonical, reverse))
        while pending:
            current = pending.pop()
            if current == canonical or current in result:
                continue
            result.add(current)
            pending.extend(self.adjacent_internal_paths(current, reverse))
        return result

    def reaches(self, source: str, target: str) -> bool:
        """Test whether source reaches target, including a zero-length path."""
        source_path = self.resolve_identifier(source)
        target_path = self.resolve_identifier(target)
        return source_path == target_path or target_path in self.reachable(source_path)

    def shortest_path(self, source: str, target: str) -> tuple[str, ...] | None:
        """Find a deterministic shortest internal include path."""
        source_path = self.resolve_identifier(source)
        target_path = self.resolve_identifier(target)
        if source_path == target_path:
            return (source_path,)
        previous: dict[str, str | None] = {source_path: None}
        pending = deque([source_path])
        while pending:
            current = pending.popleft()
            for dependency in self.adjacent_internal_paths(current):
                if dependency in previous:
                    continue
                previous[dependency] = current
                if dependency == target_path:
                    path = [dependency]
                    while previous[path[-1]] is not None:
                        path.append(previous[path[-1]])
                    return tuple(reversed(path))
                pending.append(dependency)
        return None

    def strongly_connected_components(self) -> tuple[tuple[str, ...], ...]:
        """Return every include cycle as a strongly connected component."""
        index = 0
        indices: dict[str, int] = {}
        low_links: dict[str, int] = {}
        stack: list[str] = []
        on_stack: set[str] = set()
        components: list[tuple[str, ...]] = []

        def visit(path: str) -> None:
            nonlocal index
            indices[path] = index
            low_links[path] = index
            index += 1
            stack.append(path)
            on_stack.add(path)
            for dependency in self.adjacent_internal_paths(path):
                if dependency not in indices:
                    visit(dependency)
                    low_links[path] = min(low_links[path], low_links[dependency])
                elif dependency in on_stack:
                    low_links[path] = min(low_links[path], indices[dependency])
            if low_links[path] != indices[path]:
                return
            component = []
            while True:
                dependency = stack.pop()
                on_stack.remove(dependency)
                component.append(dependency)
                if dependency == path:
                    break
            has_self_edge = len(component) == 1 and component[0] in self.adjacent_internal_paths(component[0])
            if len(component) > 1 or has_self_edge:
                components.append(tuple(sorted(component, key=str.casefold)))

        for path in sorted(self.nodes, key=str.casefold):
            if path not in indices:
                visit(path)
        return tuple(sorted(components, key=lambda component: tuple(path.casefold() for path in component)))

    def missing_required_dependency(self, rule: DependencyRule) -> tuple[str, ...]:
        """List public headers that do not transitively reach a rule target."""
        return self.analyze_required_dependency(rule).missing_headers

    def analyze_required_dependency(self, rule: DependencyRule) -> RequiredDependencyAnalysis:
        """Find all violations and a minimal leaf frontier where the dependency can be added."""
        target = self.resolve_identifier(rule.target)
        excluded = {self.resolve_identifier(path) for path in rule.excluded_headers}
        missing = {
            path
            for path, node in self.nodes.items()
            if node.public and path not in excluded and not self.reaches(path, target)
        }
        reachable_missing = {path: self.reachable(path) & missing for path in missing}
        remaining = set(missing)
        components = []
        while remaining:
            first = min(remaining, key=str.casefold)
            component = {
                path
                for path in remaining
                if path == first or (path in reachable_missing[first] and first in reachable_missing[path])
            }
            components.append(component)
            remaining -= component
        insertion_points = []
        for component in components:
            outgoing = set().union(*(reachable_missing[path] for path in component)) - component
            if not outgoing:
                insertion_points.append(min(component, key=str.casefold))
        return RequiredDependencyAnalysis(
            target=target,
            missing_headers=tuple(sorted(missing, key=str.casefold)),
            insertion_points=tuple(sorted(insertion_points, key=str.casefold)),
        )

    def relative_include_target(self, source: str, target: str) -> str:
        """Create the relative quoted-include target between two internal sources."""
        source_path = self.resolve_identifier(source)
        target_path = self.resolve_identifier(target)
        source_directory = posixpath.dirname(source_path)
        return posixpath.relpath(target_path, start=source_directory or ".")

    def summary(self) -> dict[str, Any]:
        """Create a concise graph summary document."""
        roles = Counter(node.role for node in self.nodes.values())
        return {
            "internal_nodes": len(self.nodes),
            "public_headers": sum(1 for node in self.nodes.values() if node.public),
            "internal_edges": sum(1 for edge in self.edges if edge.internal),
            "external_edges": sum(1 for edge in self.edges if not edge.internal),
            "external_headers": len(self.external_headers),
            "cycles": len(self.strongly_connected_components()),
            "roles": dict(sorted(roles.items())),
        }

    def to_document(self) -> dict[str, Any]:
        """Serialize the complete graph and cycle analysis."""
        nodes = [
            {
                "path": node.path,
                "kind": "internal",
                "role": node.role,
                "public": node.public,
                "direct_internal_dependencies": len(self.adjacent_internal_paths(node.path)),
                "transitive_internal_dependencies": len(self.reachable(node.path)),
                "direct_internal_dependents": len(self.adjacent_internal_paths(node.path, reverse=True)),
                "transitive_internal_dependents": len(self.reachable(node.path, reverse=True)),
            }
            for node in sorted(self.nodes.values(), key=lambda node: node.path.casefold())
        ]
        nodes.extend(
            {"path": f"<{path}>", "kind": "external", "role": "external", "public": False}
            for path in self.external_headers
        )
        return {
            "schema": self.SCHEMA_VERSION,
            "summary": self.summary(),
            "nodes": nodes,
            "edges": [
                {
                    "source": edge.source,
                    "target": edge.target if edge.internal else f"<{edge.target}>",
                    "line": edge.line_number,
                    "internal": edge.internal,
                    "system": edge.system,
                }
                for edge in self.edges
            ],
            "cycles": [list(component) for component in self.strongly_connected_components()],
        }

    def to_json(self) -> str:
        """Serialize the complete graph as formatted JSON."""
        return json.dumps(self.to_document(), indent=2) + "\n"

    @staticmethod
    def _dot_escape(text: str) -> str:
        """Escape one Graphviz quoted string."""
        return text.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")

    def to_dot(self) -> str:
        """Serialize the complete graph as Graphviz DOT."""
        lines = ["digraph include_dependencies {", "  rankdir=LR;"]
        for node in sorted(self.nodes.values(), key=lambda node: node.path.casefold()):
            attributes = [f'label="{self._dot_escape(node.path)}"']
            if node.public:
                attributes.append('shape="box"')
            lines.append(f'  "{self._dot_escape(node.path)}" [{", ".join(attributes)}];')
        for path in self.external_headers:
            identifier = f"<{path}>"
            lines.append(
                f'  "{self._dot_escape(identifier)}" '
                f'[label="{self._dot_escape(identifier)}", shape="ellipse", style="dashed"];'
            )
        for edge in self.edges:
            target = edge.target if edge.internal else f"<{edge.target}>"
            lines.append(
                f'  "{self._dot_escape(edge.source)}" -> "{self._dot_escape(target)}" ' f'[label="{edge.line_number}"];'
            )
        lines.append("}")
        return "\n".join(lines) + "\n"


def transitive_dependency_counts(graph: IncludeGraph) -> dict[str, int]:
    """Count reachable internal sources for every graph node."""
    return {path: len(graph.reachable(path)) for path in graph.nodes}


def summary_text(graph: IncludeGraph) -> str:
    """Render the graph summary for a terminal."""
    summary = graph.summary()
    roles = ", ".join(f"{role}={count}" for role, count in summary["roles"].items())
    return (
        f"Internal nodes: {summary['internal_nodes']} ({roles})\n"
        f"Public regular headers: {summary['public_headers']}\n"
        f"Include edges: {summary['internal_edges']} internal, {summary['external_edges']} external\n"
        f"Unique external headers: {summary['external_headers']}\n"
        f"Include cycles: {summary['cycles']}\n"
    )


def tree_text(graph: IncludeGraph, root: str, *, reverse: bool = False, max_depth: int | None = None) -> str:
    """Render one dependency or dependent tree with cycle/repetition markers."""
    canonical = graph.resolve_identifier(root)
    lines = [canonical]
    seen = {canonical}

    def visit(path: str, depth: int, active: frozenset[str]) -> None:
        if max_depth is not None and depth >= max_depth:
            if graph.dependency_edges(path, reverse):
                lines.append(f"{'  ' * (depth + 1)}... [max depth]")
            return
        for edge in graph.dependency_edges(path, reverse):
            target = edge.source if reverse else edge.target
            display = target if edge.internal else f"<{target}>"
            marker = ""
            if edge.internal and target in active:
                marker = " [cycle]"
            elif edge.internal and target in seen:
                marker = " [seen]"
            lines.append(f"{'  ' * (depth + 1)}{display}:{edge.line_number}{marker}")
            if not edge.internal or marker:
                continue
            seen.add(target)
            visit(target, depth + 1, active | {target})

    visit(canonical, 0, frozenset({canonical}))
    return "\n".join(lines) + "\n"
