#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

from lib.copyright import HeaderConfig
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.path_safety import require_directory
from lib.unicode_data import iter_ucd_data_lines, parse_code_point_range
from lib.utility import UtilityApp


@dataclass(frozen=True)
class DecompositionMapping:
    """One direct Unicode decomposition mapping."""

    code_point: int
    mapping: tuple[int, ...]
    compatibility: bool


@dataclass(frozen=True)
class NormalizationData:
    """The generated normalization tables."""

    decomposition_pages: tuple[int, ...]
    decomposition_data: bytes
    combining_class_pages: tuple[int, ...]
    combining_class_data: bytes
    composition_pages: tuple[int, ...]
    composition_data: bytes

    @property
    def total_size(self) -> int:
        """Get the encoded payload size in bytes."""
        return (
            len(self.decomposition_pages) * 4
            + len(self.decomposition_data)
            + len(self.combining_class_pages) * 4
            + len(self.combining_class_data)
            + len(self.composition_pages) * 4
            + len(self.composition_data)
        )


@dataclass(frozen=True)
class NormalizationRuntimeLimits:
    """Calculated bounds required by the stack-only normalization engine."""

    maximum_decomposition_length: int
    maximum_decomposition_depth: int
    maximum_composition_chain: int
    minimum_window_length: int


class GenerateUnicodeNormalizationDataApp(UtilityApp):
    """Generate the compact Unicode normalization tables used by the text module."""

    description = "Generate the compact Unicode normalization data tables."

    MAX_CODE_POINT = 0x10FFFF
    PAGE_SHIFT = 12
    PAGE_BITS = 9
    PAGE_MASK = (1 << PAGE_BITS) - 1
    MAX_STREAM_OFFSET = (1 << (32 - PAGE_BITS)) - 1
    MAX_MAPPING_LENGTH = 32
    MAX_RUNTIME_DECOMPOSITION_LENGTH = 18
    MAX_RUNTIME_DECOMPOSITION_DEPTH = 3
    MAX_RUNTIME_NON_STARTER_COUNT = 30
    MAX_RUNTIME_WINDOW_LENGTH = 64
    MAX_DECOMPOSITION_SIZE = 29_000
    MAX_COMBINING_CLASS_SIZE = 2_100
    MAX_COMPOSITION_SIZE = 4_700
    MAX_TOTAL_SIZE = 36_000

    def __init__(self) -> None:
        super().__init__()
        self.project_dir = Path()
        self.data_dir = Path()
        self.unicode_data_path = Path()
        self.composition_exclusions_path = Path()
        self.output_path = Path()
        self.header_config: HeaderConfig | None = None
        self.file_update = FileUpdate(self.print_verbose)

    def handle_command_line_args(self, args) -> None:
        self.project_dir = self.project_directory
        self.data_dir = self.project_dir / "utilities" / "data"
        self.unicode_data_path = self.data_dir / "UnicodeData.txt"
        self.composition_exclusions_path = self.data_dir / "CompositionExclusions.txt"
        self.output_path = self.project_dir / "src" / "erbsland" / "text" / "impl" / "UnicodeNormalizationData.cpp"

    def generated_header(self) -> str:
        """Create the generated C++ header for the normalization data file."""
        if self.header_config is None:
            raise UtilityError("Header configuration was not loaded.")
        return self.header_config.source_header("cpp", tool="generate_unicode_normalization_data.py")

    def read_unicode_data(self) -> tuple[list[DecompositionMapping], dict[int, int]]:
        """Read decomposition mappings and nonzero combining classes."""
        self.print_verbose(f"Reading Unicode normalization data from {self.unicode_data_path}")
        decompositions: list[DecompositionMapping] = []
        combining_classes: dict[int, int] = {}
        for line in iter_ucd_data_lines(self.unicode_data_path, "UnicodeData.txt"):
            fields = line.split(";")
            if len(fields) < 6:
                raise UtilityError(f"Malformed line in UnicodeData.txt: {line!r}")
            code_point = int(fields[0], 16)
            if code_point > self.MAX_CODE_POINT:
                raise UtilityError(f"UnicodeData code point exceeds U+10FFFF: U+{code_point:06X}")
            combining_class = int(fields[3])
            if not 0 <= combining_class <= 255:
                raise UtilityError(f"Combining class for U+{code_point:04X} does not fit into one byte.")
            if combining_class:
                combining_classes[code_point] = combining_class

            mapping_fields = fields[5].split()
            if not mapping_fields:
                continue
            compatibility = mapping_fields[0].startswith("<")
            if compatibility:
                if not mapping_fields[0].endswith(">"):
                    raise UtilityError(f"Malformed decomposition tag for U+{code_point:04X}.")
                mapping_fields = mapping_fields[1:]
            mapping = tuple(int(value, 16) for value in mapping_fields)
            if not mapping:
                raise UtilityError(f"Empty decomposition mapping for U+{code_point:04X}.")
            if len(mapping) > self.MAX_MAPPING_LENGTH:
                raise UtilityError(
                    f"Decomposition mapping for U+{code_point:04X} exceeds {self.MAX_MAPPING_LENGTH} code points."
                )
            if any(value > self.MAX_CODE_POINT for value in mapping):
                raise UtilityError(f"Decomposition mapping for U+{code_point:04X} exceeds U+10FFFF.")
            decompositions.append(DecompositionMapping(code_point, mapping, compatibility))

        if decompositions != sorted(decompositions, key=lambda value: value.code_point):
            raise UtilityError("Unicode decomposition mappings are not sorted by code point.")
        if len({value.code_point for value in decompositions}) != len(decompositions):
            raise UtilityError("Unicode decomposition mappings contain duplicate code points.")
        self.validate_decomposition_graph(decompositions)
        return decompositions, combining_classes

    @staticmethod
    def validate_decomposition_graph(decompositions: list[DecompositionMapping]) -> None:
        """Validate that recursive decomposition is acyclic."""
        mappings = {value.code_point: value.mapping for value in decompositions}
        completed: set[int] = set()
        active: set[int] = set()

        def visit(code_point: int) -> None:
            if code_point in completed or code_point not in mappings:
                return
            if code_point in active:
                raise UtilityError(f"Cyclic Unicode decomposition mapping at U+{code_point:04X}.")
            active.add(code_point)
            for mapped in mappings[code_point]:
                visit(mapped)
            active.remove(code_point)
            completed.add(code_point)

        for code_point in mappings:
            visit(code_point)

    def read_composition_exclusions(self) -> set[int]:
        """Read the explicit Unicode composition exclusions."""
        self.print_verbose(f"Reading composition exclusions from {self.composition_exclusions_path}")
        result: set[int] = set()
        for line in iter_ucd_data_lines(self.composition_exclusions_path, "CompositionExclusions.txt"):
            entry_range = parse_code_point_range(line.strip())
            if entry_range.end > self.MAX_CODE_POINT:
                raise UtilityError("Composition exclusion exceeds U+10FFFF.")
            result.update(range(entry_range.begin, entry_range.end + 1))
        return result

    @classmethod
    def validate_runtime_limits(
        cls,
        decompositions: list[DecompositionMapping],
        combining_classes: dict[int, int],
        compositions: dict[int, list[tuple[int, int]]],
    ) -> NormalizationRuntimeLimits:
        """Validate the fixed buffers and source-aligned boundary assumptions used at runtime."""
        mappings = {value.code_point: value for value in decompositions}
        expansion_cache: dict[tuple[int, bool], tuple[int, ...]] = {}
        depth_cache: dict[tuple[int, bool], int] = {}

        def expand(code_point: int, compatibility: bool) -> tuple[int, ...]:
            key = (code_point, compatibility)
            if key in expansion_cache:
                return expansion_cache[key]
            entry = mappings.get(code_point)
            if entry is None or (entry.compatibility and not compatibility):
                result = (code_point,)
            else:
                result = tuple(mapped for value in entry.mapping for mapped in expand(value, compatibility))
            expansion_cache[key] = result
            return result

        def depth(code_point: int, compatibility: bool) -> int:
            key = (code_point, compatibility)
            if key in depth_cache:
                return depth_cache[key]
            entry = mappings.get(code_point)
            if entry is None or (entry.compatibility and not compatibility):
                result = 0
            else:
                result = 1 + max((depth(value, compatibility) for value in entry.mapping), default=0)
            depth_cache[key] = result
            return result

        maximum_length = 3  # Algorithmic Hangul decomposition.
        maximum_depth = 1
        for compatibility in (False, True):
            for code_point in mappings:
                expanded = expand(code_point, compatibility)
                maximum_length = max(maximum_length, len(expanded))
                maximum_depth = max(maximum_depth, depth(code_point, compatibility))
                if combining_classes.get(expanded[0], 0) != 0 and any(
                    combining_classes.get(value, 0) == 0 for value in expanded[1:]
                ):
                    raise UtilityError(
                        f"Recursive decomposition for U+{code_point:04X} begins with a non-starter "
                        "and later contains a starter."
                    )

        if maximum_length > cls.MAX_RUNTIME_DECOMPOSITION_LENGTH:
            raise UtilityError(
                "Recursive Unicode decomposition exceeds the runtime buffer of "
                f"{cls.MAX_RUNTIME_DECOMPOSITION_LENGTH} code points."
            )
        if maximum_depth > cls.MAX_RUNTIME_DECOMPOSITION_DEPTH:
            raise UtilityError(
                "Recursive Unicode decomposition exceeds the runtime depth of "
                f"{cls.MAX_RUNTIME_DECOMPOSITION_DEPTH}."
            )

        composition_graph = {
            starter: tuple(composite for trailing, composite in pairs if combining_classes.get(trailing, 0) == 0)
            for starter, pairs in compositions.items()
        }
        composition_depths: dict[int, int] = {}
        active_compositions: set[int] = set()

        def composition_chain_length(starter: int) -> int:
            if starter in composition_depths:
                return composition_depths[starter]
            if starter in active_compositions:
                raise UtilityError(f"Cyclic class-zero composition chain at U+{starter:04X}.")
            active_compositions.add(starter)
            result = 1 + max(
                (composition_chain_length(composite) for composite in composition_graph.get(starter, ())), default=0
            )
            active_compositions.remove(starter)
            composition_depths[starter] = result
            return result

        maximum_composition_chain = 3  # Algorithmic Hangul composition: L + V + T.
        for starter in composition_graph:
            maximum_composition_chain = max(maximum_composition_chain, composition_chain_length(starter))

        # A source scalar can contribute `maximum_length` normalized scalars. Alternatively, class-zero source
        # scalars can remain aligned while they successively compose. Either case can then be followed by the
        # maximum accepted run of non-starters.
        minimum_window_length = max(maximum_length, maximum_composition_chain) + cls.MAX_RUNTIME_NON_STARTER_COUNT
        if minimum_window_length > cls.MAX_RUNTIME_WINDOW_LENGTH:
            raise UtilityError(
                f"Unicode normalization requires a runtime window of {minimum_window_length} code points, "
                f"exceeding the configured {cls.MAX_RUNTIME_WINDOW_LENGTH}."
            )
        return NormalizationRuntimeLimits(
            maximum_length, maximum_depth, maximum_composition_chain, minimum_window_length
        )

    @staticmethod
    def encode_unsigned(value: int) -> bytes:
        """Encode an unsigned integer as canonical ULEB128."""
        if value < 0:
            raise UtilityError("Cannot encode a negative ULEB128 value.")
        result = bytearray()
        while True:
            byte = value & 0x7F
            value >>= 7
            if value:
                byte |= 0x80
            result.append(byte)
            if not value:
                return bytes(result)

    @classmethod
    def pack_page(cls, page: int, offset: int) -> int:
        """Pack a page number and byte offset into one 32-bit directory entry."""
        if not 0 <= page <= cls.PAGE_MASK:
            raise UtilityError(f"Unicode normalization page does not fit into {cls.PAGE_BITS} bits: {page}")
        if not 0 <= offset <= cls.MAX_STREAM_OFFSET:
            raise UtilityError(f"Unicode normalization stream offset is too large: {offset}")
        return (offset << cls.PAGE_BITS) | page

    @classmethod
    def encode_decompositions(cls, mappings: list[DecompositionMapping]) -> tuple[tuple[int, ...], bytes]:
        """Encode direct decomposition mappings into sparse pages."""
        pages: dict[int, list[DecompositionMapping]] = defaultdict(list)
        for mapping in mappings:
            pages[mapping.code_point >> cls.PAGE_SHIFT].append(mapping)

        directory: list[int] = []
        data = bytearray()
        for page, entries in sorted(pages.items()):
            directory.append(cls.pack_page(page, len(data)))
            previous = page << cls.PAGE_SHIFT
            for entry in entries:
                data.extend(cls.encode_unsigned(entry.code_point - previous))
                header = len(entry.mapping) - 1
                if entry.compatibility:
                    header |= 0x20
                data.append(header)
                for mapped in entry.mapping:
                    data.extend(cls.encode_unsigned(mapped))
                previous = entry.code_point
        return tuple(directory), bytes(data)

    @classmethod
    def encode_combining_classes(cls, combining_classes: dict[int, int]) -> tuple[tuple[int, ...], bytes]:
        """Encode nonzero canonical combining classes into sparse pages."""
        pages: dict[int, list[tuple[int, int]]] = defaultdict(list)
        for code_point, combining_class in sorted(combining_classes.items()):
            pages[code_point >> cls.PAGE_SHIFT].append((code_point, combining_class))

        directory: list[int] = []
        data = bytearray()
        for page, entries in sorted(pages.items()):
            directory.append(cls.pack_page(page, len(data)))
            previous = page << cls.PAGE_SHIFT
            for code_point, combining_class in entries:
                data.extend(cls.encode_unsigned(code_point - previous))
                data.append(combining_class)
                previous = code_point
        return tuple(directory), bytes(data)

    @classmethod
    def build_compositions(
        cls,
        decompositions: list[DecompositionMapping],
        combining_classes: dict[int, int],
        exclusions: set[int],
    ) -> dict[int, list[tuple[int, int]]]:
        """Build the canonical composition pairs grouped by starter."""
        result: dict[int, list[tuple[int, int]]] = defaultdict(list)
        seen_pairs: set[tuple[int, int]] = set()
        for entry in decompositions:
            if entry.compatibility or len(entry.mapping) != 2:
                continue
            first, second = entry.mapping
            if entry.code_point in exclusions:
                continue
            if combining_classes.get(entry.code_point, 0) != 0 or combining_classes.get(first, 0) != 0:
                continue
            pair = (first, second)
            if pair in seen_pairs:
                raise UtilityError(f"Duplicate canonical composition pair U+{first:04X} U+{second:04X}.")
            seen_pairs.add(pair)
            result[first].append((second, entry.code_point))
        for pairs in result.values():
            pairs.sort()
        return result

    @classmethod
    def encode_compositions(cls, compositions: dict[int, list[tuple[int, int]]]) -> tuple[tuple[int, ...], bytes]:
        """Encode canonical composition pairs into sparse starter pages."""
        pages: dict[int, list[tuple[int, list[tuple[int, int]]]]] = defaultdict(list)
        for starter, pairs in sorted(compositions.items()):
            pages[starter >> cls.PAGE_SHIFT].append((starter, pairs))

        directory: list[int] = []
        data = bytearray()
        for page, entries in sorted(pages.items()):
            directory.append(cls.pack_page(page, len(data)))
            previous_starter = page << cls.PAGE_SHIFT
            for starter, pairs in entries:
                data.extend(cls.encode_unsigned(starter - previous_starter))
                data.extend(cls.encode_unsigned(len(pairs)))
                previous_trailing = 0
                for trailing, composite in pairs:
                    data.extend(cls.encode_unsigned(trailing - previous_trailing))
                    data.extend(cls.encode_unsigned(composite))
                    previous_trailing = trailing
                previous_starter = starter
        return tuple(directory), bytes(data)

    def generate_data(self) -> NormalizationData:
        """Read UCD inputs and build all encoded normalization tables."""
        decompositions, combining_classes = self.read_unicode_data()
        exclusions = self.read_composition_exclusions()
        compositions = self.build_compositions(decompositions, combining_classes, exclusions)
        self.validate_runtime_limits(decompositions, combining_classes, compositions)
        decomposition_pages, decomposition_data = self.encode_decompositions(decompositions)
        combining_class_pages, combining_class_data = self.encode_combining_classes(combining_classes)
        composition_pages, composition_data = self.encode_compositions(compositions)
        result = NormalizationData(
            decomposition_pages,
            decomposition_data,
            combining_class_pages,
            combining_class_data,
            composition_pages,
            composition_data,
        )
        self.validate_size_budgets(result)
        return result

    @classmethod
    def validate_size_budgets(cls, data: NormalizationData) -> None:
        """Validate the expected Unicode normalization size budgets."""
        if len(data.decomposition_data) > cls.MAX_DECOMPOSITION_SIZE:
            raise UtilityError("Unicode decomposition data exceeds its 29 KB size budget.")
        if len(data.combining_class_data) > cls.MAX_COMBINING_CLASS_SIZE:
            raise UtilityError("Unicode combining-class data exceeds its 2.1 KB size budget.")
        if len(data.composition_data) > cls.MAX_COMPOSITION_SIZE:
            raise UtilityError("Unicode composition data exceeds its 4.7 KB size budget.")
        if data.total_size > cls.MAX_TOTAL_SIZE:
            raise UtilityError("Unicode normalization data exceeds its 36 KB total size budget.")

    @staticmethod
    def render_array(values: bytes | tuple[int, ...], value_type: str, name: str) -> list[str]:
        """Render one generated C++ array."""
        width = 16 if value_type == "uint8_t" else 6
        digits = 2 if value_type == "uint8_t" else 8
        lines = [
            "    // clang-format off",
            f"    static constexpr auto data = std::array<{value_type}, {len(values)}>{{",
        ]
        for begin in range(0, len(values), width):
            chunk = values[begin : begin + width]
            rendered = ", ".join(f"0x{value:0{digits}X}U" for value in chunk)
            lines.append(f"        {rendered},")
        lines.extend(["    };", "    // clang-format on", f"    return data; // {name}"])
        return lines

    def render_cpp(self, data: NormalizationData) -> str:
        """Render the generated normalization data translation unit."""
        sections = [
            ("unicodeNormalizationDecompositionPages", data.decomposition_pages, "uint32_t", "decomposition pages"),
            ("unicodeNormalizationDecompositionData", data.decomposition_data, "uint8_t", "decomposition data"),
            (
                "unicodeNormalizationCombiningClassPages",
                data.combining_class_pages,
                "uint32_t",
                "combining-class pages",
            ),
            (
                "unicodeNormalizationCombiningClassData",
                data.combining_class_data,
                "uint8_t",
                "combining-class data",
            ),
            ("unicodeNormalizationCompositionPages", data.composition_pages, "uint32_t", "composition pages"),
            ("unicodeNormalizationCompositionData", data.composition_data, "uint8_t", "composition data"),
        ]
        lines = [
            self.generated_header(),
            "",
            '#include "UnicodeNormalizationData.hpp"',
            "",
            "#include <array>",
            "",
            "namespace erbsland::text::impl {",
            "",
        ]
        for function_name, values, value_type, label in sections:
            lines.append(f"auto {function_name}() noexcept -> std::span<const {value_type}> {{")
            lines.extend(self.render_array(values, value_type, label))
            lines.extend(["}", ""])
        lines.extend(["}", ""])
        return "\n".join(lines)

    def run(self, argv=None) -> None:
        super().run(argv)
        self.header_config = HeaderConfig.read(self.config_file_path())
        require_directory(self.data_dir, "Unicode normalization data directory")
        data = self.generate_data()
        self.print_verbose(
            "Unicode normalization payload: "
            f"decomposition={len(data.decomposition_data)}, "
            f"combining={len(data.combining_class_data)}, "
            f"composition={len(data.composition_data)}, total={data.total_size} bytes"
        )
        self.file_update.write_if_changed(self.output_path, self.render_cpp(data))


def main() -> None:
    raise SystemExit(GenerateUnicodeNormalizationDataApp().main())


if __name__ == "__main__":
    main()
