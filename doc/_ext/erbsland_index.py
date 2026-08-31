# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import re
from dataclasses import dataclass

from sphinx import addnodes
from sphinx.application import Sphinx
from sphinx.transforms import SphinxTransform

_ERBSLAND_CPP_INDEX_RE = re.compile(r"^(?P<name>erbsland::.+?) (?P<suffix>\(C\+\+ [^)]+\))$")
_CPP_TARGET_PREFIX = "_CPP"
_CPP_ENUMERATOR_SUFFIX = "(C++ enumerator)"
_CPP_FUNCTION_SUFFIX = "(C++ function)"
_CPP_MEMBER_SUFFIX = "(C++ member)"


@dataclass(frozen=True)
class ErbslandCppIndexValue:
    """Parsed Erbsland C++ index value."""

    parts: tuple[str, ...]
    suffix: str

    @property
    def leaf_name(self) -> str:
        """The final C++ name component."""
        return self.parts[-1]

    @property
    def is_destructor(self) -> bool:
        """Test if this index value points to a destructor."""
        return self.leaf_name.startswith("~")

    @property
    def is_constructor(self) -> bool:
        """Test if this index value points to a constructor."""
        return self.suffix == _CPP_FUNCTION_SUFFIX and self.parent_name == self.leaf_name

    @property
    def parent_name(self) -> str | None:
        """Return the direct parent name for class-like members."""
        if len(self.parts) < 4:
            return None
        return self.parts[-2]

    @property
    def subentry_parent_name(self) -> str | None:
        """Return a class-like parent name for index subentries."""
        parent_name = self.parent_name
        if parent_name is None or not parent_name[:1].isupper():
            return None
        if self.suffix != _CPP_FUNCTION_SUFFIX or len(self.parts) < 4:
            if self.suffix in {_CPP_ENUMERATOR_SUFFIX, _CPP_MEMBER_SUFFIX}:
                return parent_name
            return None
        if self.leaf_name in {parent_name, f"~{parent_name}"}:
            return None
        return parent_name


def strip_erbsland_cpp_index_value(value: str) -> str:
    """Strip Erbsland Core namespaces and parent classes from a C++ index value."""
    parsed_value = parse_erbsland_cpp_index_value(value)
    if parsed_value is None:
        return value
    if not parsed_value.leaf_name:
        return value
    return f"{parsed_value.leaf_name} {parsed_value.suffix}"


def parse_erbsland_cpp_index_value(value: str) -> ErbslandCppIndexValue | None:
    """Parse a generated Erbsland C++ index value."""
    match = _ERBSLAND_CPP_INDEX_RE.match(value)
    if match is None:
        return None
    parts = tuple(part for part in match.group("name").split("::") if part)
    if len(parts) < 3 or parts[0] != "erbsland":
        return None
    return ErbslandCppIndexValue(parts, match.group("suffix"))


def parent_subentry_index_entry(
    entry: tuple[str, str, str, str, str | None],
) -> tuple[str, str, str, str, str | None] | None:
    """Create a parent subentry if a generated entry describes a nested API item."""
    entry_type, value, target_id, main, category_key = entry
    parsed_value = parse_erbsland_cpp_index_value(value)
    if parsed_value is None:
        return None
    parent_name = parsed_value.subentry_parent_name
    if parent_name is None:
        return None
    return (entry_type, f"{parsed_value.leaf_name}; {parent_name}", target_id, main, category_key)


def is_generated_cpp_index_entry(entry: tuple[str, str, str, str, str | None]) -> bool:
    """Test if an index entry was generated for a C++ object."""
    entry_type, value, target_id, _main, _category_key = entry
    return entry_type == "single" and value.startswith("erbsland::") and target_id.startswith(_CPP_TARGET_PREFIX)


class ErbslandIndexTransform(SphinxTransform):
    """Rewrite generated Erbsland C++ index entries for leaf-name lookup."""

    default_priority = 849

    def apply(self, **kwargs) -> None:
        """Rewrite index node entries before Sphinx domains collect them."""
        for node in self.document.findall(addnodes.index):
            new_entries = []
            changed = False
            for entry in node["entries"]:
                extra_entry = None
                if len(entry) == 5 and is_generated_cpp_index_entry(entry):
                    entry_type, value, target_id, main, category_key = entry
                    parsed_value = parse_erbsland_cpp_index_value(value)
                    if parsed_value is not None and (parsed_value.is_constructor or parsed_value.is_destructor):
                        changed = True
                        continue
                    extra_entry = parent_subentry_index_entry(entry)
                    if extra_entry is not None:
                        new_entries.append(extra_entry)
                        changed = True
                        continue
                    new_value = strip_erbsland_cpp_index_value(value)
                    if new_value != value:
                        entry = (entry_type, new_value, target_id, main, category_key)
                        changed = True
                new_entries.append(entry)
            if changed:
                node["entries"] = new_entries


def setup(app: Sphinx) -> dict[str, bool]:
    """Register Erbsland Core index transforms."""
    app.add_transform(ErbslandIndexTransform)
    return {
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
