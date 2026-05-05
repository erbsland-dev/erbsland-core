#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

from lib.copyright import HeaderConfig
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.path_safety import read_safe_text, require_directory
from lib.unicode_data import CodePointRange, iter_ucd_data_lines, parse_code_point_range
from lib.utility import UtilityApp


@dataclass
class UnicodeRangeEntry:
    code_point: int
    category: str
    delta_index: int = 0
    display_width: int = 1


@dataclass(frozen=True)
class UnicodeDelta:
    case_fold: int = 0
    lowercase: int = 0
    uppercase: int = 0


class GenerateUnicodeLightDataApp(UtilityApp):
    """Generate the compact Unicode Light tables used by the text module."""

    description = "Generate the compact Unicode Light data tables."

    MAX_CODE_POINT = 0x10FFFF
    UNICODE_LIMIT = MAX_CODE_POINT + 1
    CATEGORY_ENUM_NAMES = {
        "Cc": "UnicodeCategory::Control",
        "Cf": "UnicodeCategory::Format",
        "Cn": "UnicodeCategory::Unassigned",
        "Co": "UnicodeCategory::PrivateUse",
        "Cs": "UnicodeCategory::Surrogate",
        "Ll": "UnicodeCategory::LowercaseLetter",
        "Lm": "UnicodeCategory::ModifierLetter",
        "Lo": "UnicodeCategory::OtherLetter",
        "Lt": "UnicodeCategory::TitlecaseLetter",
        "Lu": "UnicodeCategory::UppercaseLetter",
        "Mc": "UnicodeCategory::SpacingMark",
        "Me": "UnicodeCategory::EnclosingMark",
        "Mn": "UnicodeCategory::NonspacingMark",
        "Nd": "UnicodeCategory::DecimalNumber",
        "Nl": "UnicodeCategory::LetterNumber",
        "No": "UnicodeCategory::OtherNumber",
        "Pc": "UnicodeCategory::ConnectorPunctuation",
        "Pd": "UnicodeCategory::DashPunctuation",
        "Pe": "UnicodeCategory::ClosePunctuation",
        "Pf": "UnicodeCategory::FinalPunctuation",
        "Pi": "UnicodeCategory::InitialPunctuation",
        "Po": "UnicodeCategory::OtherPunctuation",
        "Ps": "UnicodeCategory::OpenPunctuation",
        "Sc": "UnicodeCategory::CurrencySymbol",
        "Sk": "UnicodeCategory::ModifierSymbol",
        "Sm": "UnicodeCategory::MathSymbol",
        "So": "UnicodeCategory::OtherSymbol",
        "Zl": "UnicodeCategory::LineSeparator",
        "Zp": "UnicodeCategory::ParagraphSeparator",
        "Zs": "UnicodeCategory::SpaceSeparator",
    }
    VERSION_PATTERN = re.compile(r"Version (\d+)\.(\d+)\.(\d+)")

    def __init__(self) -> None:
        super().__init__()
        self.project_dir = Path()
        self.data_dir = Path()
        self.unicode_data_path = Path()
        self.case_folding_path = Path()
        self.east_asian_width_path = Path()
        self.readme_path = Path()
        self.output_path = Path()
        self.version_output_path = Path()
        self.header_config: HeaderConfig | None = None
        self.file_update = FileUpdate(self.print_verbose)

    def handle_command_line_args(self, args) -> None:
        self.project_dir = self.project_directory
        self.data_dir = self.project_dir / "utilities" / "data"
        self.unicode_data_path = self.data_dir / "UnicodeData.txt"
        self.case_folding_path = self.data_dir / "CaseFolding.txt"
        self.east_asian_width_path = self.data_dir / "EastAsianWidth.txt"
        self.readme_path = self.data_dir / "ReadMe.txt"
        self.output_path = self.project_dir / "src" / "erbsland" / "text" / "impl" / "UnicodeData.cpp"
        self.version_output_path = self.project_dir / "src" / "erbsland" / "text" / "impl" / "UnicodeVersion.cpp"

    def generated_header(self) -> str:
        """Create the generated C++ header for Unicode Light data files."""
        if self.header_config is None:
            raise UtilityError("Header configuration was not loaded.")
        return self.header_config.source_header("cpp", tool="generate_unicode_light_data.py")

    def read_unicode_data(self) -> tuple[list[str], list[int], list[int]]:
        self.print_verbose(f"Reading UnicodeData from {self.unicode_data_path}")
        text = read_safe_text(self.unicode_data_path, "UnicodeData.txt")
        general_categories = ["Cn"] * (self.MAX_CODE_POINT + 1)
        lowercase_deltas = [0] * (self.MAX_CODE_POINT + 1)
        uppercase_deltas = [0] * (self.MAX_CODE_POINT + 1)
        first_range_start: int | None = None
        first_range_category: str | None = None
        first_range_upper: str = ""
        first_range_lower: str = ""
        for raw_line in text.splitlines():
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            fields = line.split(";")
            if len(fields) < 14:
                raise UtilityError(f"Malformed line in UnicodeData.txt: {raw_line!r}")
            code_point = int(fields[0], 16)
            if code_point > self.MAX_CODE_POINT:
                continue
            name = fields[1]
            category = fields[2]
            uppercase_mapping = fields[12]
            lowercase_mapping = fields[13]
            if name.endswith(", First>"):
                if first_range_start is not None:
                    raise UtilityError("Nested <First> ranges are not supported in UnicodeData.txt.")
                first_range_start = code_point
                first_range_category = category
                first_range_upper = uppercase_mapping
                first_range_lower = lowercase_mapping
                continue
            if name.endswith(", Last>"):
                if first_range_start is None or first_range_category is None:
                    raise UtilityError("Encountered <Last> without matching <First> in UnicodeData.txt.")
                if category != first_range_category:
                    raise UtilityError("Mismatched categories between <First> and <Last> UnicodeData entries.")
                for current_code_point in range(first_range_start, code_point + 1):
                    general_categories[current_code_point] = category
                    if first_range_upper:
                        uppercase_deltas[current_code_point] = int(first_range_upper, 16) - current_code_point
                    if first_range_lower:
                        lowercase_deltas[current_code_point] = int(first_range_lower, 16) - current_code_point
                first_range_start = None
                first_range_category = None
                first_range_upper = ""
                first_range_lower = ""
                continue
            general_categories[code_point] = category
            if uppercase_mapping:
                uppercase_deltas[code_point] = int(uppercase_mapping, 16) - code_point
            if lowercase_mapping:
                lowercase_deltas[code_point] = int(lowercase_mapping, 16) - code_point
        if first_range_start is not None:
            raise UtilityError("Unterminated <First> range in UnicodeData.txt.")
        return general_categories, lowercase_deltas, uppercase_deltas

    def read_case_fold_deltas(self) -> list[int]:
        self.print_verbose(f"Reading CaseFolding data from {self.case_folding_path}")
        text = read_safe_text(self.case_folding_path, "CaseFolding.txt")
        deltas = [0] * (self.MAX_CODE_POINT + 1)
        for raw_line in text.splitlines():
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            parts = [part.strip() for part in line.split(";")]
            if len(parts) < 3:
                raise UtilityError(f"Malformed line in CaseFolding.txt: {raw_line!r}")
            source = int(parts[0], 16)
            if source > self.MAX_CODE_POINT:
                continue
            if parts[1] not in {"C", "S"}:
                continue
            mapping = [token for token in parts[2].split() if token]
            if len(mapping) != 1:
                continue
            destination = int(mapping[0], 16)
            if destination > self.MAX_CODE_POINT:
                continue
            deltas[source] = destination - source
        return deltas

    @classmethod
    def apply_width_range(cls, widths: bytearray, entry_range: CodePointRange, width: int) -> None:
        """Apply a terminal-cell width to a Unicode range."""
        if entry_range.end >= cls.UNICODE_LIMIT:
            raise UtilityError(f"Unicode range exceeds U+10FFFF: {entry_range.begin:04X}..{entry_range.end:04X}")
        widths[entry_range.begin : entry_range.end + 1] = bytes([width]) * (entry_range.end - entry_range.begin + 1)

    @staticmethod
    def is_zero_width_category(general_category: str) -> bool:
        """Test if a Unicode general category has terminal-cell width zero."""
        return general_category.startswith("M") or general_category in {"Cc", "Cf"}

    def read_display_widths(self) -> bytearray:
        """Load terminal-cell width data from UCD files."""
        self.print_verbose(f"Reading EastAsianWidth from {self.east_asian_width_path}")
        widths = bytearray([1]) * self.UNICODE_LIMIT

        for line in iter_ucd_data_lines(self.east_asian_width_path, "EastAsianWidth.txt"):
            code_point_text, width_class = [part.strip() for part in line.split(";", maxsplit=1)]
            if width_class in {"W", "F"}:
                self.apply_width_range(widths, parse_code_point_range(code_point_text), 2)

        self.print_verbose(f"Reading Unicode category widths from {self.unicode_data_path}")
        first_range: CodePointRange | None = None
        first_range_category = ""
        for line in iter_ucd_data_lines(self.unicode_data_path, "UnicodeData.txt"):
            fields = line.split(";")
            if len(fields) < 3:
                raise UtilityError(f"Malformed line in UnicodeData.txt: {line!r}")
            code_point = int(fields[0], 16)
            name = fields[1]
            general_category = fields[2]
            if name.endswith(", First>"):
                if first_range is not None:
                    raise UtilityError("Nested <First> ranges are not supported in UnicodeData.txt.")
                first_range = CodePointRange(code_point, code_point)
                first_range_category = general_category
                continue
            if name.endswith(", Last>"):
                if first_range is None:
                    raise UtilityError("Encountered <Last> without matching <First> in UnicodeData.txt.")
                if general_category != first_range_category:
                    raise UtilityError("Mismatched categories between <First> and <Last> UnicodeData entries.")
                if self.is_zero_width_category(general_category):
                    self.apply_width_range(widths, CodePointRange(first_range.begin, code_point), 0)
                first_range = None
                first_range_category = ""
                continue
            if self.is_zero_width_category(general_category):
                widths[code_point] = 0
        if first_range is not None:
            raise UtilityError("Unterminated <First> range in UnicodeData.txt.")
        return widths

    def unique_delta_table(
        self, case_fold_deltas: list[int], lowercase_deltas: list[int], uppercase_deltas: list[int]
    ) -> tuple[list[int], list[UnicodeDelta]]:
        unique_deltas = [UnicodeDelta()]
        delta_index_by_value = {UnicodeDelta(): 0}
        indices = []
        for case_fold_delta, lowercase_delta, uppercase_delta in zip(
            case_fold_deltas, lowercase_deltas, uppercase_deltas, strict=True
        ):
            delta = UnicodeDelta(case_fold_delta, lowercase_delta, uppercase_delta)
            if delta not in delta_index_by_value:
                delta_index_by_value[delta] = len(unique_deltas)
                unique_deltas.append(delta)
            indices.append(delta_index_by_value[delta])
        if len(unique_deltas) > 256:
            raise UtilityError(f"Unicode delta triples use too many entries for an 8-bit table: {len(unique_deltas)}")
        return indices, unique_deltas

    def parse_ucd_version(self) -> tuple[int, int, int]:
        self.print_verbose(f"Reading Unicode version from {self.readme_path}")
        text = read_safe_text(self.readme_path, "ReadMe.txt")
        match = self.VERSION_PATTERN.search(text)
        if match is None:
            raise UtilityError("Failed to parse the Unicode version from utilities/data/ReadMe.txt.")
        return tuple(int(value) for value in match.groups())

    def compress_to_ranges(
        self,
        general_categories: list[str],
        delta_indices: list[int],
        display_widths: bytearray,
    ) -> list[UnicodeRangeEntry]:
        ranges: list[UnicodeRangeEntry] = []
        previous: tuple[str, int, int] | None = None
        for code_point in range(self.MAX_CODE_POINT + 1):
            current = (
                general_categories[code_point],
                delta_indices[code_point],
                display_widths[code_point],
            )
            if previous is None or current != previous:
                ranges.append(
                    UnicodeRangeEntry(
                        code_point,
                        current[0],
                        current[1],
                        current[2],
                    )
                )
                previous = current
        ranges.append(UnicodeRangeEntry(self.MAX_CODE_POINT + 1, "Cn"))
        return ranges

    def render_unicode_data_cpp(
        self,
        ranges: list[UnicodeRangeEntry],
        delta_table: list[UnicodeDelta],
    ) -> str:
        def data_for(code_point: int) -> UnicodeRangeEntry:
            for index, entry in enumerate(ranges):
                next_code_point = ranges[index + 1].code_point
                if entry.code_point <= code_point < next_code_point:
                    return UnicodeRangeEntry(
                        code_point,
                        entry.category,
                        entry.delta_index,
                        entry.display_width,
                    )
            raise UtilityError(f"Failed to find generated Unicode data for U+{code_point:04X}.")

        def format_build(entry: UnicodeRangeEntry) -> str:
            category = self.CATEGORY_ENUM_NAMES[entry.category]
            if entry.delta_index or entry.display_width != 1:
                return (
                    f"        build(0x{entry.code_point:06X}U, {category}, "
                    f"0x{entry.delta_index:02X}U, {entry.display_width}U),"
                )
            return f"        build(0x{entry.code_point:06X}U, {category}),"

        ascii_entries = [data_for(code_point) for code_point in range(128)]
        non_ascii_entries = [entry for entry in ranges if entry.code_point >= 0x80]
        if not non_ascii_entries or non_ascii_entries[0].code_point != 0x80:
            non_ascii_entries.insert(0, data_for(0x80))
        lines = [
            self.generated_header(),
            "",
            '#include "UnicodeData.hpp"',
            "",
            "",
            "#include <array>",
            "",
            "namespace erbsland::text::impl {",
            "",
            "constexpr auto build(",
            "    const uint32_t codePoint,",
            "    const UnicodeCategory category,",
            "    const uint8_t deltaIndex = 0U,",
            "    const uint8_t displayWidth = 1U) noexcept -> UnicodeData {",
            "",
            "    return UnicodeData{",
            "        static_cast<UnicodeData::CodePoint>(codePoint),",
            "        category,",
            "        deltaIndex,",
            "        displayWidth};",
            "}",
            "",
            "auto asciiUnicodeDataTable() noexcept -> std::span<const UnicodeData> {",
            "    static constexpr auto table = std::array<const UnicodeData, 128>{",
        ]
        for entry in ascii_entries:
            lines.append(format_build(entry))
        lines.extend(
            [
                "    };",
                "    return table;",
                "}",
                "",
                "auto unicodeDataMap() noexcept -> std::span<const UnicodeData> {",
                f"    static const auto data = std::array<const UnicodeData, {len(non_ascii_entries)}>{{",
            ]
        )
        for entry in non_ascii_entries:
            lines.append(format_build(entry))
        lines.extend(
            [
                "    };",
                "    return data;",
                "}",
                "",
                "auto unicodeDeltaTable() noexcept -> std::span<const UnicodeDelta> {",
                f"    static constexpr std::array<UnicodeDelta, {len(delta_table)}> table{{",
            ]
        )
        for value in delta_table:
            lines.append(f"        UnicodeDelta{{{value.case_fold}, {value.lowercase}, {value.uppercase}}},")
        lines.extend(
            [
                "    };",
                "    return table;",
                "}",
                "",
                "}",
                "",
            ]
        )
        return "\n".join(lines)

    def render_unicode_version_cpp(self, version: tuple[int, int, int]) -> str:
        major, minor, revision = version
        return "\n".join(
            [
                self.generated_header(),
                "",
                '#include "UnicodeData.hpp"',
                "",
                "",
                "namespace erbsland::text::impl {",
                "",
                "auto unicodeDataVersion() noexcept -> unit::Version {",
                f"    return unit::Version{{{major}, {minor}, {revision}}};",
                "}",
                "",
                "}",
                "",
            ]
        )

    def run(self, argv=None) -> None:
        super().run(argv)
        self.header_config = HeaderConfig.read(self.config_file_path())
        require_directory(self.data_dir, "Unicode Light data directory")
        general_categories, lowercase_deltas, uppercase_deltas = self.read_unicode_data()
        case_fold_deltas = self.read_case_fold_deltas()
        display_widths = self.read_display_widths()
        delta_indices, delta_table = self.unique_delta_table(case_fold_deltas, lowercase_deltas, uppercase_deltas)
        ranges = self.compress_to_ranges(
            general_categories,
            delta_indices,
            display_widths,
        )
        unicode_data_cpp = self.render_unicode_data_cpp(
            ranges,
            delta_table,
        )
        version_cpp = self.render_unicode_version_cpp(self.parse_ucd_version())
        self.file_update.write_if_changed(self.output_path, unicode_data_cpp)
        self.file_update.write_if_changed(self.version_output_path, version_cpp)


def main() -> None:
    raise SystemExit(GenerateUnicodeLightDataApp().main())


if __name__ == "__main__":
    main()
