#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import idna
from idna import idnadata

from lib.copyright import HeaderConfig
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.unicode_data import iter_ucd_data_lines, parse_code_point_range
from lib.utility import UtilityApp


@dataclass(frozen=True)
class RangeValue:
    """One inclusive code-point range and its compact property value."""

    first: int
    last: int
    value: int


class GenerateIdnaDataApp(UtilityApp):
    """Generate Unicode 17 IDNA2008 runtime tables."""

    description = "Generate compact Unicode 17 IDNA2008 runtime tables."
    MAX_CODE_POINT = 0x10FFFF
    STATUS_SHIFT = 0
    BIDI_SHIFT = 2
    JOINING_SHIFT = 6
    VIRAMA_SHIFT = 9
    SCRIPT_SHIFT = 10

    def __init__(self) -> None:
        super().__init__()
        self.data_dir = Path()
        self.output_path = Path()
        self.header_config: HeaderConfig | None = None
        self.file_update = FileUpdate(self.print_verbose)

    def handle_command_line_args(self, args) -> None:
        self.data_dir = self.project_directory / "utilities" / "data"
        self.output_path = self.project_directory / "src" / "erbsland" / "text" / "punycode" / "impl" / "IdnaData.cpp"

    def generated_header(self) -> str:
        """Create the generated C++ header."""
        if self.header_config is None:
            raise UtilityError("Header configuration was not loaded.")
        return self.header_config.source_header("cpp", tool="generate_idna_data.py")

    @staticmethod
    def decode_intranges(values: tuple[int, ...], property_value: int) -> list[RangeValue]:
        """Decode the compact range representation used by the independent idna package."""
        return [RangeValue(value >> 32, (value & 0xFFFFFFFF) - 1, property_value) for value in values]

    @staticmethod
    def merge_ranges(values: list[RangeValue]) -> list[RangeValue]:
        """Merge adjacent ranges with the same property value."""
        result: list[RangeValue] = []
        for value in sorted(values, key=lambda item: (item.first, item.last)):
            if result and result[-1].value == value.value and result[-1].last + 1 == value.first:
                result[-1] = RangeValue(result[-1].first, value.last, value.value)
            else:
                result.append(value)
        return result

    def read_unicode_data(self) -> tuple[dict[int, str], dict[int, int], dict[int, str]]:
        """Read general category, combining class, and bidi class from UnicodeData.txt."""
        categories: dict[int, str] = {}
        combining: dict[int, int] = {}
        bidi: dict[int, str] = {}
        pending: tuple[int, list[str]] | None = None
        for line in iter_ucd_data_lines(self.data_dir / "UnicodeData.txt", "UnicodeData.txt"):
            fields = line.split(";")
            code_point = int(fields[0], 16)
            if fields[1].endswith(", First>"):
                pending = (code_point, fields)
                continue
            if fields[1].endswith(", Last>"):
                if pending is None:
                    raise UtilityError("UnicodeData range end without a range start.")
                first, first_fields = pending
                if first_fields[2:5] != fields[2:5]:
                    raise UtilityError("UnicodeData range properties do not match.")
                for value in range(first, code_point + 1):
                    categories[value] = fields[2]
                    combining[value] = int(fields[3])
                    bidi[value] = fields[4]
                pending = None
                continue
            categories[code_point] = fields[2]
            combining[code_point] = int(fields[3])
            bidi[code_point] = fields[4]
        if pending is not None:
            raise UtilityError("Unterminated UnicodeData range.")
        return categories, combining, bidi

    def read_scripts(self) -> dict[str, list[RangeValue]]:
        """Read the scripts required by RFC 5892 contextual rules."""
        wanted = {"Greek": 1, "Han": 2, "Hebrew": 3, "Hiragana": 4, "Katakana": 5}
        result = {name: [] for name in wanted}
        for line in iter_ucd_data_lines(self.data_dir / "Scripts.txt", "Scripts.txt"):
            fields = [part.strip() for part in line.split(";")]
            script = fields[1]
            if script not in wanted:
                continue
            code_range = parse_code_point_range(fields[0])
            result[script].append(RangeValue(code_range.begin, code_range.end, wanted[script]))
        return result

    def read_joining_types(self, categories: dict[int, str]) -> list[RangeValue]:
        """Derive Joining_Type, including Unicode default transparent marks."""
        joining: dict[int, int] = {}
        values = {"L": 1, "R": 2, "D": 3, "T": 4}
        for code_point, category in categories.items():
            if category in {"Mn", "Me", "Cf"}:
                joining[code_point] = values["T"]
        for line in iter_ucd_data_lines(self.data_dir / "ArabicShaping.txt", "ArabicShaping.txt"):
            fields = [part.strip() for part in line.split(";")]
            code_point = int(fields[0], 16)
            joining_type = fields[2]
            if joining_type in values:
                joining[code_point] = values[joining_type]
            else:
                joining.pop(code_point, None)
        joining.pop(0x200C, None)
        joining.pop(0x200D, None)
        return self.map_to_ranges(joining)

    @staticmethod
    def map_to_ranges(values: dict[int, int]) -> list[RangeValue]:
        """Convert a sparse code-point value map into merged ranges."""
        result: list[RangeValue] = []
        for code_point, value in sorted(values.items()):
            if result and result[-1].value == value and result[-1].last + 1 == code_point:
                result[-1] = RangeValue(result[-1].first, code_point, value)
            else:
                result.append(RangeValue(code_point, code_point, value))
        return result

    def status_ranges(self) -> list[RangeValue]:
        """Read Unicode 17 RFC 5892 derived status ranges from the pinned independent implementation."""
        if idna.__version__ != "3.18" or idnadata.__version__ != "17.0.0":
            raise UtilityError("generate_idna_data requires idna==3.18 with Unicode 17.0.0 data.")
        result: list[RangeValue] = []
        for name, value in (("PVALID", 1), ("CONTEXTJ", 2), ("CONTEXTO", 3)):
            result.extend(self.decode_intranges(idnadata.codepoint_classes[name], value))
        return sorted(result, key=lambda item: item.first)

    def property_ranges(self) -> dict[str, list[RangeValue]]:
        """Build all runtime property ranges from local Unicode source files."""
        categories, combining, bidi = self.read_unicode_data()
        bidi_values = {
            "L": 1,
            "R": 2,
            "AL": 3,
            "EN": 4,
            "AN": 5,
            "ES": 6,
            "CS": 7,
            "ET": 8,
            "ON": 9,
            "BN": 10,
            "NSM": 11,
        }
        bidi_map = {code_point: bidi_values[value] for code_point, value in bidi.items() if value in bidi_values}
        virama_map = {code_point: 1 for code_point, value in combining.items() if value == 9}
        scripts = self.read_scripts()
        result = {
            "idnaStatusRanges": self.status_ranges(),
            "idnaBidiRanges": self.map_to_ranges(bidi_map),
            "idnaJoiningRanges": self.read_joining_types(categories),
            "idnaViramaRanges": self.map_to_ranges(virama_map),
        }
        for script, ranges in scripts.items():
            result[f"idna{script}Ranges"] = self.merge_ranges(ranges)
        return result

    @staticmethod
    def split_at_page_boundaries(values: list[RangeValue]) -> list[RangeValue]:
        """Split ranges at 16-bit Unicode page boundaries."""
        result: list[RangeValue] = []
        for value in values:
            first = value.first
            while first <= value.last:
                last = min(value.last, ((first >> 16) + 1) * 0x10000 - 1)
                result.append(RangeValue(first, last, value.value))
                first = last + 1
        return result

    def runtime_ranges(self, data: dict[str, list[RangeValue]]) -> list[RangeValue]:
        """Merge all properties required at runtime into one compact attribute table."""
        attributes: dict[int, int] = {}
        for value in data["idnaStatusRanges"]:
            for code_point in range(value.first, value.last + 1):
                attributes[code_point] = value.value << self.STATUS_SHIFT

        properties = (
            ("idnaBidiRanges", self.BIDI_SHIFT),
            ("idnaJoiningRanges", self.JOINING_SHIFT),
            ("idnaViramaRanges", self.VIRAMA_SHIFT),
            ("idnaGreekRanges", self.SCRIPT_SHIFT),
            ("idnaHanRanges", self.SCRIPT_SHIFT),
            ("idnaHebrewRanges", self.SCRIPT_SHIFT),
            ("idnaHiraganaRanges", self.SCRIPT_SHIFT),
            ("idnaKatakanaRanges", self.SCRIPT_SHIFT),
        )
        for name, shift in properties:
            for value in data[name]:
                encoded = value.value << shift
                for code_point in range(value.first, value.last + 1):
                    if code_point in attributes:
                        attributes[code_point] |= encoded

        result = self.split_at_page_boundaries(self.map_to_ranges(attributes))
        for value in result:
            if value.first >> 16 != value.last >> 16:
                raise UtilityError("A compact IDNA range crosses a 16-bit Unicode page boundary.")
            if value.value & 0x0003 == 0:
                raise UtilityError("A compact IDNA range contains no valid derived status.")
            if value.value > 0x1FFF:
                raise UtilityError("The compact IDNA attributes exceed their 13-bit representation.")
        return result

    @staticmethod
    def page_offsets(ranges: list[RangeValue]) -> list[int]:
        """Create range offsets for every represented 16-bit Unicode page."""
        if not ranges:
            return [0]
        if len(ranges) > 0xFFFF:
            raise UtilityError("The compact IDNA range count exceeds a 16-bit page offset.")
        page_count = (ranges[-1].last >> 16) + 1
        result: list[int] = []
        range_index = 0
        for page in range(page_count):
            result.append(range_index)
            while range_index < len(ranges) and ranges[range_index].first >> 16 == page:
                range_index += 1
        result.append(range_index)
        return result

    @staticmethod
    def render_function(ranges: list[RangeValue], page_offsets: list[int]) -> list[str]:
        """Render the compact generated lookup function."""
        lines = [
            "auto idnaAttributes(const char32_t codePoint) noexcept -> IdnaAttributes {",
            "    // clang-format off",
            f"    static constexpr auto data = std::array<IdnaRange, {len(ranges)}>{{{{",
        ]
        for item in ranges:
            lines.append(
                f"        {{0x{item.first & 0xFFFF:04X}U, 0x{item.last & 0xFFFF:04X}U, "
                f"IdnaAttributes{{0x{item.value:04X}U}}}},"
            )
        offsets = ", ".join(f"{value}U" for value in page_offsets)
        lines.extend(
            [
                "    }};",
                f"    static constexpr auto pageOffsets = std::array<uint16_t, {len(page_offsets)}>{{{{{offsets}}}}};",
                "    // clang-format on",
                "",
                "    const auto page = static_cast<std::size_t>(codePoint >> 16U);",
                "    if (page + 1U >= pageOffsets.size()) {",
                "        return {};",
                "    }",
                "    const auto pageCodePoint = static_cast<uint16_t>(codePoint & 0xFFFFU);",
                "    auto first = static_cast<std::size_t>(pageOffsets[page]);",
                "    auto last = static_cast<std::size_t>(pageOffsets[page + 1U]);",
                "    while (first < last) {",
                "        const auto middle = first + (last - first) / 2U;",
                "        if (pageCodePoint < data[middle].first) {",
                "            last = middle;",
                "        } else if (pageCodePoint > data[middle].last) {",
                "            first = middle + 1U;",
                "        } else {",
                "            return data[middle].attributes;",
                "        }",
                "    }",
                "    return {};",
                "}",
                "",
            ]
        )
        return lines

    def render_cpp(self, data: dict[str, list[RangeValue]]) -> str:
        """Render the generated translation unit."""
        lines = [
            self.generated_header(),
            "",
            '#include "IdnaData.hpp"',
            "",
            "#include <array>",
            "",
            "namespace erbsland::text::punycode::impl {",
            "",
        ]
        ranges = self.runtime_ranges(data)
        lines.extend(self.render_function(ranges, self.page_offsets(ranges)))
        lines.extend(["}", ""])
        return "\n".join(lines)

    def run(self, argv=None) -> None:
        super().run(argv)
        self.header_config = HeaderConfig.read(self.config_file_path())
        required = (
            "ArabicShaping.txt",
            "Blocks.txt",
            "DerivedCoreProperties.txt",
            "HangulSyllableType.txt",
            "PropList.txt",
            "Scripts.txt",
            "UnicodeData.txt",
        )
        missing = [name for name in required if not (self.data_dir / name).is_file()]
        if missing:
            raise UtilityError(f"Missing Unicode source files: {', '.join(missing)}")
        data = self.property_ranges()
        self.file_update.write_if_changed(self.output_path, self.render_cpp(data))


def main() -> None:
    raise SystemExit(GenerateIdnaDataApp().main())


if __name__ == "__main__":
    main()
