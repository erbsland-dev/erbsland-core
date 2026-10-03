# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Use Doxygen's index to avoid Breathe lookups through unrelated XML files."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from breathe.directives.class_like import DoxygenClassDirective, DoxygenInterfaceDirective, DoxygenStructDirective
from breathe.directives.function import DoxygenFunctionDirective
from breathe.directives.item import DoxygenConceptDirective, DoxygenEnumDirective, DoxygenTypedefDirective
from breathe.finder.factory import Finder, FinderFactory, _FakeParentNode
from breathe.renderer.filter import Filter, FilterFactory
from sphinx.application import Sphinx


@dataclass(frozen=True)
class SymbolQuery:
    """Lookup information retained before Breathe constructs its filter tree."""

    node_type: str
    name: str
    kinds: tuple[str, ...]
    namespace: str = ""


class IndexedFilter(Filter):
    """Keep Breathe's predicate authoritative while exposing candidate lookup keys."""

    def __init__(self, original: Filter, query: SymbolQuery) -> None:
        self.original = original
        self.query = query

    def allow(self, node_stack) -> bool:
        return self.original.allow(node_stack)


class SymbolFilterFactory(FilterFactory):
    """Attach lookup keys without interpreting Breathe's internal filter expressions."""

    def create_compound_finder_filter(self, name: str, kind: str) -> IndexedFilter:
        return IndexedFilter(super().create_compound_finder_filter(name, kind), SymbolQuery("compound", name, (kind,)))

    def create_member_finder_filter(self, namespace: str, name: str, kind: str) -> IndexedFilter:
        return IndexedFilter(
            super().create_member_finder_filter(namespace, name, kind),
            SymbolQuery("member", name, (kind,), namespace),
        )

    def create_function_and_all_friend_finder_filter(self, namespace: str, name: str) -> IndexedFilter:
        return IndexedFilter(
            super().create_function_and_all_friend_finder_filter(namespace, name),
            SymbolQuery("member", name, ("function", "friend"), namespace),
        )


class SymbolIndex:
    """Name maps owned by one parsed index.xml, with Breathe's cache lifetime.

    Buckets retain duplicate compounds. Member buckets contain each owner once;
    Breathe resolves all matching overloads inside that owner's XML. Ordinals
    preserve XML order when merging function and friend candidates.
    """

    def __init__(self, root) -> None:
        self.compounds: dict[tuple[str, str], list[Any]] = {}
        self.members: dict[tuple[str, str, str], list[tuple[int, Any]]] = {}
        self.unqualified_members: dict[tuple[str, str], list[tuple[int, Any]]] = {}
        for ordinal, compound in enumerate(root.get_compound()):
            self.compounds.setdefault((compound.kind, compound.name), []).append(compound)
            keys = {(member.kind, member.name) for member in compound.get_member()}
            for kind, name in keys:
                candidate = (ordinal, compound)
                self.members.setdefault((compound.name, kind, name), []).append(candidate)
                self.unqualified_members.setdefault((kind, name), []).append(candidate)

    def candidates(self, query: SymbolQuery) -> list[Any]:
        if query.node_type == "compound":
            return self.compounds.get((query.kinds[0], query.name), [])
        owners = {}
        for kind in query.kinds:
            if query.namespace:
                candidates = self.members.get((query.namespace, kind, query.name), ())
            else:
                candidates = self.unqualified_members.get((kind, query.name), ())
            owners.update(candidates)
        return [owners[ordinal] for ordinal in sorted(owners)]


class CompoundIndexFinder(Finder):
    """Apply index-based filters without traversing unrelated compound files.

    Breathe's class-like directives filter by the compound's exact name and kind.
    Both are already in index.xml; nodes inside compound files cannot match that
    filter. Member lookups select matching index members first, then delegate to
    Breathe to resolve their full definitions. Keep its node stacks and rendering.
    This finder must not be used for file or enumvalue filters: those match nodes
    inside compound files which are not described by the index.
    """

    def __init__(self, root, item_finder_factory) -> None:
        super().__init__(root, item_finder_factory)
        # Finder factories are recreated for every directive. The parsed root is
        # shared across pages, isolated per XML project and replaced on a new build.
        if not hasattr(root, "_erbsland_symbol_index"):
            root._erbsland_symbol_index = SymbolIndex(root)
        self.symbol_index = root._erbsland_symbol_index

    def filter_(self, filter_, matches) -> None:
        if not isinstance(filter_, IndexedFilter):
            # Unsupported predicates retain Breathe's complete traversal semantics.
            super().filter_(filter_, matches)
            return
        ancestors = [self.root(), _FakeParentNode()]
        for compound in self.symbol_index.candidates(filter_.query):
            node_stack = [compound, *ancestors]
            if filter_.allow(node_stack):
                matches.append(node_stack)
            elif any(filter_.allow([member, *node_stack]) for member in compound.get_member()):
                finder = self.item_finder_factory.create_finder(compound)
                finder.filter_(ancestors, filter_, matches)


class CompoundIndexFinderFactory(FinderFactory):
    def create_finder(self, project_info) -> CompoundIndexFinder:
        finder = super().create_finder(project_info)
        return CompoundIndexFinder(finder.root(), finder.item_finder_factory)


class CompoundIndexDirective:
    @property
    def finder_factory(self) -> CompoundIndexFinderFactory:
        return CompoundIndexFinderFactory(self.env.app, self.parser_factory)

    @property
    def filter_factory(self) -> SymbolFilterFactory:
        return SymbolFilterFactory(self.env.app)


class IndexedClassDirective(CompoundIndexDirective, DoxygenClassDirective):
    pass


class IndexedStructDirective(CompoundIndexDirective, DoxygenStructDirective):
    pass


class IndexedInterfaceDirective(CompoundIndexDirective, DoxygenInterfaceDirective):
    pass


class IndexedConceptDirective(CompoundIndexDirective, DoxygenConceptDirective):
    pass


class IndexedEnumDirective(CompoundIndexDirective, DoxygenEnumDirective):
    pass


class IndexedTypedefDirective(CompoundIndexDirective, DoxygenTypedefDirective):
    pass


class IndexedFunctionDirective(CompoundIndexDirective, DoxygenFunctionDirective):
    pass


def setup(app: Sphinx) -> dict[str, bool | str]:
    """Override only directives whose lookup is entirely described by index.xml."""
    app.setup_extension("breathe")
    app.add_directive("doxygenclass", IndexedClassDirective, override=True)
    app.add_directive("doxygenstruct", IndexedStructDirective, override=True)
    app.add_directive("doxygeninterface", IndexedInterfaceDirective, override=True)
    app.add_directive("doxygenconcept", IndexedConceptDirective, override=True)
    app.add_directive("doxygenenum", IndexedEnumDirective, override=True)
    app.add_directive("doxygentypedef", IndexedTypedefDirective, override=True)
    app.add_directive("doxygenfunction", IndexedFunctionDirective, override=True)
    return {"version": "1", "parallel_read_safe": True, "parallel_write_safe": True}
