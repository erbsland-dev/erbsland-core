# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import re
import subprocess
import xml.etree.ElementTree as et
from pathlib import Path

from lib.copyright import HeaderConfig
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.path_safety import read_safe_text, require_directory
from lib.time_zone.model import Rule, RuleSet, TimeReference, Zone, ZoneEntry, epoch_seconds


class TimeZoneGenerator:
    """Parse the vendored tzdb files and generate the C++ compact database."""

    VERSION_PATTERN = re.compile(r"^(\d{4})([a-z])$")
    REGION_FILES = [
        "africa",
        "antarctica",
        "asia",
        "australasia",
        "europe",
        "northamerica",
        "southamerica",
        "etcetera",
        "backward",
    ]
    CLDR_WINDOWS_ZONES_FILE = "windowsZones.xml"

    def __init__(
        self,
        project_dir: Path,
        data_dir: Path,
        output_dir: Path,
        file_update: FileUpdate,
        header_config: HeaderConfig,
    ) -> None:
        self.project_dir = project_dir
        self.data_dir = data_dir
        self.output_dir = output_dir
        self.file_update = file_update
        self.header_config = header_config
        self.version_year = 0
        self.version_revision = 0
        self.rule_sets: dict[str, RuleSet] = {}
        self.zones: dict[str, Zone] = {}
        self.links: dict[str, str] = {}
        self.zone_list: list[Zone] = []
        self.texts: list[str] = [""]
        self.generated_files: list[Path] = []

    def run(self) -> list[Path]:
        require_directory(self.data_dir, "tzdb data directory")
        self.read_database()
        self.build_texts()
        self.write_text_files()
        self.write_rule_files()
        self.write_zone_files()
        self.write_database_version()
        self.write_windows_time_zone_map()
        return self.generated_files

    def write_windows_time_zone_map(self) -> None:
        """Generate the Windows key to canonical world-territory IANA mapping from Unicode CLDR."""
        try:
            source_path = self.project_dir / "utilities" / "data" / self.CLDR_WINDOWS_ZONES_FILE
            root = et.fromstring(read_safe_text(source_path, "Unicode CLDR Windows time-zone data"))
        except et.ParseError as error:
            raise UtilityError(f"Failed to parse Unicode CLDR Windows time-zone data: {error}") from error
        mappings = sorted(
            (
                node.attrib["other"],
                node.attrib["type"].split()[0],
            )
            for node in root.findall(".//mapZone")
            if node.attrib.get("territory") == "001"
        )
        entries = []
        for windows_name, iana_name in mappings:
            target_name = self.links.get(iana_name, iana_name)
            if target_name not in self.zones:
                raise UtilityError(f'CLDR time-zone name "{iana_name}" is not present in the bundled IANA database.')
            entries.append(
                f'        Mapping{{L"{windows_name}", TimeZoneId{{0x{self.zones[target_name].id:04x}U}}}},'
                f" // {iana_name}"
            )
        self.write(
            "WindowsTimeZoneMap.hpp",
            f'''{self.header(pragma_once=True)}

#include "../../TimeZoneId.hpp"

#include <optional>
#include <string_view>

namespace erbsland::time::tz::impl {{

/// Map a Windows time-zone key to the canonical CLDR world-territory IANA zone identifier.
/// @param windowsName The Windows time-zone key.
/// @return The time-zone identifier, or no value for an unknown key.
[[nodiscard]] auto timeZoneIdFromWindowsName(std::wstring_view windowsName) noexcept -> std::optional<TimeZoneId>;

}}
''',
        )
        self.write(
            "WindowsTimeZoneMap.cpp",
            f'''{self.header()}
#include "WindowsTimeZoneMap.hpp"

#include <array>

namespace erbsland::time::tz::impl {{

namespace {{
struct Mapping {{
    std::wstring_view windowsName;
    TimeZoneId timeZoneId;
}};

// Generated from Unicode CLDR windowsZones.xml entries for territory "001".
constexpr auto cMappings = std::to_array<Mapping>({{
{chr(10).join(entries)}
}});
}}

auto timeZoneIdFromWindowsName(const std::wstring_view windowsName) noexcept -> std::optional<TimeZoneId> {{
    for (const auto &mapping : cMappings) {{
        if (mapping.windowsName == windowsName) {{
            return mapping.timeZoneId;
        }}
    }}
    return {{}};
}}

}}
''',
        )

    def read_database(self) -> None:
        self.parse_version()
        for region_name in self.REGION_FILES:
            self.parse_region(region_name)
        for index, name in enumerate(sorted(self.zones)):
            zone = self.zones[name]
            zone.id = index + 1
            self.zone_list.append(zone)
            for entry in zone.entries:
                if entry.is_rule_set_based:
                    if entry.rule_set_name not in self.rule_sets:
                        raise UtilityError(f'Zone "{zone.name}" references unknown rule set "{entry.rule_set_name}".')
                    entry.rule_set = self.rule_sets[entry.rule_set_name]
        for link, target in sorted(self.links.items()):
            if target not in self.zones:
                raise UtilityError(f'Link "{link}" references unknown target "{target}".')

    def parse_version(self) -> None:
        text = read_safe_text(self.data_dir / "version", "tzdb version").strip()
        match = self.VERSION_PATTERN.match(text)
        if match is None:
            raise UtilityError(f"Unsupported tzdb version: {text!r}")
        self.version_year = int(match.group(1))
        self.version_revision = ord(match.group(2)) - ord("a") + 1

    def parse_region(self, region_name: str) -> None:
        text = read_safe_text(self.data_dir / region_name, f"tzdb {region_name}")
        zone_lines: list[str] = []
        for raw_line in text.splitlines():
            line = raw_line.split("#", 1)[0].rstrip()
            if not line.strip():
                continue
            stripped = line.strip()
            if stripped.startswith("Rule"):
                self.add_rule(stripped)
                continue
            if stripped.startswith("Link"):
                self.add_link(stripped)
                continue
            if stripped.startswith("Zone"):
                if zone_lines:
                    self.add_zone(zone_lines, region_name)
                    zone_lines = []
                zone_lines.append(stripped)
                continue
            if zone_lines:
                zone_lines.append(stripped)
        if zone_lines:
            self.add_zone(zone_lines, region_name)

    def add_rule(self, line: str) -> None:
        rule = Rule.parse(line)
        self.rule_sets.setdefault(rule.name, RuleSet(rule.name)).rules.append(rule)

    def add_link(self, line: str) -> None:
        parts = line.split()
        if len(parts) < 3:
            raise UtilityError(f"Unsupported link line: {line}")
        self.links[parts[2]] = parts[1]

    def add_zone(self, lines: list[str], region_name: str) -> None:
        parts = lines[0].split()
        name = parts[1]
        zone = self.zones.setdefault(name, Zone(name, region_name))
        zone.entries.extend(ZoneEntry.parse(line) for line in lines)

    def build_texts(self) -> None:
        text_set: set[str] = set()
        for name in list(self.zones) + list(self.links):
            parts = name.split("/")
            if len(parts) > 3:
                raise UtilityError(f'Zone name "{name}" has more than three path elements.')
            text_set.update(parts)
        for zone in self.zones.values():
            text_set.update(zone.all_abbreviations())
        for text in text_set:
            if len(text) > 15:
                raise UtilityError(f'Text "{text}" exceeds the generated table length limit.')
        self.texts = ["", *sorted(text_set)]

    def write_text_files(self) -> None:
        text_block = ""
        entries = [(0, 0)]
        for text in self.texts[1:]:
            offset = text_block.find(text)
            if offset < 0:
                offset = len(text_block)
                text_block += text
            entries.append((offset, len(text)))
        if len(text_block) > 0xFFFF:
            raise UtilityError("Generated text block exceeds the 16-bit storage limit.")
        all_names = sorted([*self.zones, *self.links])
        zone_names = [self.zone_name_entry(name) for name in all_names]
        primary_names = [self.zone_name_entry(zone.name) for zone in self.zone_list]
        self.write(
            "Texts.hpp",
            f"""{self.header(pragma_once=True)}

#include "Text.hpp"
#include "ZoneName.hpp"

#include <array>
#include <string_view>

namespace erbsland::time::tz::impl {{

[[nodiscard]] auto textEntries() noexcept -> const std::array<Text, {len(entries)}>&;
[[nodiscard]] auto textBlock() noexcept -> std::string_view;
[[nodiscard]] auto zoneNames() noexcept -> const std::array<ZoneName, {len(zone_names)}>&;
[[nodiscard]] auto primaryZoneNames() noexcept -> const std::array<ZoneName, {len(primary_names)}>&;

}}
""",
        )
        self.write(
            "Texts.cpp",
            f"""{self.header()}
#include "Texts.hpp"

namespace erbsland::time::tz::impl {{

auto textEntries() noexcept -> const std::array<Text, {len(entries)}>& {{
    static constexpr auto entries = std::array<Text, {len(entries)}>{{
{self.format_text_entries(entries)}
    }};
    return entries;
}}

auto textBlock() noexcept -> std::string_view {{
    static constexpr auto block = std::string_view{{
{self.format_text_block(text_block)}
    }};
    return block;
}}

auto zoneNames() noexcept -> const std::array<ZoneName, {len(zone_names)}>& {{
    static constexpr auto names = std::array<ZoneName, {len(zone_names)}>{{
{self.format_zone_names(zone_names)}
    }};
    return names;
}}

auto primaryZoneNames() noexcept -> const std::array<ZoneName, {len(primary_names)}>& {{
    static constexpr auto names = std::array<ZoneName, {len(primary_names)}>{{
{self.format_zone_names(primary_names)}
    }};
    return names;
}}

}}
""",
        )

    def write_rule_files(self) -> None:
        for index, name in enumerate(sorted(self.rule_sets)):
            self.rule_sets[name].id = index
        encoded_rules: list[tuple[int, str, str]] = []
        offsets = []
        for name in sorted(self.rule_sets):
            offsets.append(len(encoded_rules))
            for rule in self.rule_sets[name].rules:
                value, comment = rule.encoded()
                encoded_rules.append((value, comment, name))
        offsets.append(len(encoded_rules))
        encoded_rules.append((0, "// dummy", ""))
        self.write(
            "Rules.hpp",
            f"""{self.header(pragma_once=True)}

#include "RuleSet.hpp"

#include <cstdint>

namespace erbsland::time::tz::impl {{

[[nodiscard]] auto ruleSet(uint16_t index) noexcept -> RuleSet;

}}
""",
        )
        rule_lines = []
        last_name = None
        for value, comment, name in encoded_rules:
            if name and name != last_name:
                rule_lines.append(f"        // {name}")
                last_name = name
            rule_lines.append(f"        Rule{{0x{value:016x}ULL}}, {comment}")
        offset_lines = ", ".join(f"{offset}U" for offset in offsets)
        self.write(
            "Rules.cpp",
            f"""{self.header()}
#include "Rules.hpp"

#include <array>

namespace erbsland::time::tz::impl {{

namespace {{

constexpr auto cRules = std::array<Rule, {len(encoded_rules)}>{{
{chr(10).join(rule_lines)}
}};

constexpr auto cRuleSetOffsets = std::array<std::size_t, {len(offsets)}>{{{offset_lines}}};

}}

auto ruleSet(uint16_t index) noexcept -> RuleSet {{
    if (index + 1U >= cRuleSetOffsets.size()) {{
        return {{std::span<const Rule>{{cRules.data(), 0}}}};
    }}
    const auto begin = cRuleSetOffsets[index];
    const auto end = cRuleSetOffsets[index + 1U];
    return {{std::span<const Rule>{{cRules.data() + begin, end - begin}}}};
}}

}}
""",
        )

    def write_zone_files(self) -> None:
        used_names: set[str] = set()
        region_decl: dict[str, list[str]] = {name: [] for name in self.REGION_FILES if name != "backward"}
        region_impl: dict[str, list[str]] = {name: [] for name in self.REGION_FILES if name != "backward"}
        info_functions: dict[int, str] = {}
        for zone in self.zone_list:
            zone.function_name = self.unique_function_name(zone.name, used_names)
            info_functions[zone.id] = zone.function_name
            declaration, implementation = self.zone_code(zone)
            region_decl[zone.region].append(declaration)
            region_impl[zone.region].append(implementation)
        for region, declarations in region_decl.items():
            class_name = self.region_class_name(region)
            self.write(
                f"Region{class_name}.hpp",
                f"""{self.header(pragma_once=True)}

#include "Info.hpp"

#include <memory>

namespace erbsland::time::tz::impl {{

{chr(10).join(declarations)}

}}
""",
            )
            self.write(
                f"Region{class_name}.cpp",
                f"""{self.header()}
#include "Region{class_name}.hpp"

#include "Rules.hpp"

namespace erbsland::time::tz::impl {{

{chr(10).join(region_impl[region])}
}}
""",
            )
        region_includes = "\n".join(f'#include "Region{self.region_class_name(region)}.hpp"' for region in sorted(region_decl))
        function_lines = []
        for zone_id in range(1, len(self.zone_list) + 1):
            function_lines.append(f"        &{info_functions[zone_id]},")
        self.write(
            "Zones.hpp",
            f"""{self.header(pragma_once=True)}

#include "Info.hpp"
#include "ZoneId.hpp"

#include <memory>

namespace erbsland::time::tz::impl {{

[[nodiscard]] auto info(ZoneId id) noexcept -> std::unique_ptr<Info>;

}}
""",
        )
        self.write(
            "Zones.cpp",
            f"""{self.header()}
#include "Zones.hpp"

{region_includes}

#include <array>

namespace erbsland::time::tz::impl {{

namespace {{

using InfoFunction = auto (*)() noexcept -> std::unique_ptr<Info>;

constexpr auto cInfoFunctions = std::array<InfoFunction, {len(self.zone_list)}>{{
{chr(10).join(function_lines)}
}};

}}

auto info(ZoneId id) noexcept -> std::unique_ptr<Info> {{
    if (id == cUtcZoneId || id == cZoneIdNotFound || id - 1U >= cInfoFunctions.size()) {{
        return nullptr;
    }}
    return cInfoFunctions[id - 1U]();
}}

}}
""",
        )

    def write_database_version(self) -> None:
        self.write(
            "Database_version.cpp",
            f"""{self.header()}
#include "Database.hpp"

namespace erbsland::time::tz::impl {{

auto Database::version() noexcept -> unit::Version {{
    return unit::Version{{1, {self.version_year}, {self.version_revision}}};
}}

}}
""",
        )

    def zone_code(self, zone: Zone) -> tuple[str, str]:
        abbreviation_lists, abbreviation_ids, used_rule_sets = self.zone_abbreviations(zone)
        abbreviation_rule_ids, abbreviation_rule_offsets = self.pack_abbreviation_ids(abbreviation_lists)
        text_ids = sorted({value for value in [*abbreviation_ids, *abbreviation_rule_ids] if value != 0})
        if len(text_ids) > 0x1F:
            raise UtilityError(f'Zone "{zone.name}" uses too many abbreviations.')
        used_rule_names = [rule_set.name for rule_set in used_rule_sets]
        rule_sets_code = ", ".join(f"ruleSet({rule_set.id}U) /* {rule_set.name} */" for rule_set in used_rule_sets)
        text_ids_code = ", ".join(f"0x{value:04x}U" for value in text_ids)
        abbreviation_map = []
        for value in abbreviation_rule_ids:
            abbreviation_map.append(f"0x{text_ids.index(value) + 1:02x}U" if value in text_ids else "0x00U")
        lines = []
        for index, entry in enumerate(zone.entries):
            fixed_id = abbreviation_ids[index]
            fixed_offset = text_ids.index(fixed_id) + 1 if fixed_id in text_ids else 0
            lines.append(self.continuation_line_code(entry, fixed_offset, abbreviation_rule_offsets[index], used_rule_names))
        declaration = f"[[nodiscard]] auto {zone.function_name}() noexcept -> std::unique_ptr<Info>;"
        implementation = f"""auto {zone.function_name}() noexcept -> std::unique_ptr<Info> {{
    return std::make_unique<Info>(
        std::vector<RuleSet>{{{rule_sets_code}}},
        std::vector<ContinuationLine>{{
{chr(10).join(lines)}
        }},
        std::vector<TextId>{{{text_ids_code}}},
        std::vector<AbbreviationOffset>{{{", ".join(abbreviation_map)}}});
}}

"""
        return declaration, implementation

    def zone_abbreviations(self, zone: Zone) -> tuple[list[list[int]], list[int], list[RuleSet]]:
        used_rules: dict[str, RuleSet] = {}
        abbreviation_lists: list[list[int]] = []
        abbreviation_ids: list[int] = []
        for entry in zone.entries:
            if entry.is_rule_set_based:
                assert entry.rule_set is not None
                used_rules.setdefault(entry.rule_set.name, entry.rule_set)
            is_dynamic_abbreviation = entry.is_rule_set_based and ("%s" in entry.format_text or "%z" in entry.format_text)
            if not is_dynamic_abbreviation:
                fixed_rule_offset = 0 if entry.is_rule_set_based else entry.rule_offset_seconds
                text = entry.abbreviation_text("", entry.std_offset_seconds + fixed_rule_offset)
                abbreviation_ids.append(self.text_id(text) if text else 0)
                abbreviation_lists.append([])
                continue
            if not entry.is_rule_set_based or entry.rule_set is None:
                abbreviation_ids.append(0)
                abbreviation_lists.append([])
                continue
            current_list = [
                self.text_id(entry.abbreviation_text(rule.letters, entry.std_offset_seconds + rule.save_minutes * 60))
                for rule in entry.rule_set.rules
            ]
            non_zero = {value for value in current_list if value != 0}
            if len(non_zero) == 1:
                abbreviation_ids.append(next(iter(non_zero)))
                abbreviation_lists.append([])
            else:
                abbreviation_ids.append(0)
                abbreviation_lists.append(current_list)
        return abbreviation_lists, abbreviation_ids, sorted(used_rules.values(), key=lambda rule_set: rule_set.name)

    @staticmethod
    def pack_abbreviation_ids(abbreviation_lists: list[list[int]]) -> tuple[list[int], list[int]]:
        packed: list[int] = []
        offsets: list[int] = []
        for abbreviation_list in abbreviation_lists:
            if not abbreviation_list:
                offsets.append(0)
                continue
            found = -1
            for index in range(len(packed)):
                tested = packed[index : index + len(abbreviation_list)]
                if tested == abbreviation_list[: len(tested)]:
                    found = index
                    if index + len(abbreviation_list) > len(packed):
                        packed[index : index + len(abbreviation_list)] = abbreviation_list
                    break
            if found < 0:
                found = len(packed)
                packed.extend(abbreviation_list)
            offsets.append(found + 1)
        return packed, offsets

    def continuation_line_code(
        self, entry: ZoneEntry, abbreviation_id: int, abbreviation_offset: int, used_rule_names: list[str]
    ) -> str:
        standard_offset = entry.std_offset_seconds
        if entry.is_rule_set_based:
            rule_id = used_rule_names.index(entry.rule_set_name)
            rule_or_offset = 0x10000 + rule_id + (abbreviation_offset << 4)
        else:
            rule_or_offset = entry.rule_offset_seconds
        if entry.until_text:
            until_dt, until_reference = entry.parse_until()
            utc_until_dt, utc_until_offset = entry.last_effective_offset(until_dt, until_reference)
            until_seconds = epoch_seconds(utc_until_dt)
        else:
            until_seconds = 0
            utc_until_offset = 0
        return (
            f"            ContinuationLine{{{standard_offset:>7}, 0x{until_seconds:010x}ULL, "
            f"{utc_until_offset:>7}, {rule_or_offset:>7}, 0x{abbreviation_id:02x}U}},"
        )

    def zone_name_entry(self, name: str) -> tuple[int, int, int, int]:
        target = self.links.get(name, name)
        zone = self.zones[target]
        parts = name.split("/")
        ids = [self.text_id(part) for part in parts]
        while len(ids) < 3:
            ids.append(0)
        return ids[0], ids[1], ids[2], zone.id

    def text_id(self, text: str) -> int:
        if not text:
            return 0
        return self.texts.index(text)

    @staticmethod
    def unique_function_name(name: str, used_names: set[str]) -> str:
        result = "info"
        for char in name:
            if char.isalnum():
                result += char
            elif char == "/":
                result += "_"
            elif char == "+":
                result += "Plus"
            elif char == "-":
                result += "_"
        if result in used_names:
            suffix = 2
            while f"{result}_{suffix}" in used_names:
                suffix += 1
            result = f"{result}_{suffix}"
        used_names.add(result)
        return result

    @staticmethod
    def region_class_name(region_name: str) -> str:
        return "".join(part.capitalize() for part in region_name.split("_"))

    @staticmethod
    def format_text_entries(entries: list[tuple[int, int]]) -> str:
        return "\n".join(f"        Text{{0x{offset:04x}U, 0x{length:02x}U}}," for offset, length in entries)

    @staticmethod
    def format_text_block(text_block: str) -> str:
        chunks = [text_block[index : index + 96] for index in range(0, len(text_block), 96)]
        return "\n".join(f'        "{chunk}"' for chunk in chunks)

    @staticmethod
    def format_zone_names(zone_names: list[tuple[int, int, int, int]]) -> str:
        return "\n".join(
            f"        ZoneName{{0x{text1:04x}U, 0x{text2:04x}U, 0x{text3:04x}U, 0x{zone_id:04x}U}},"
            for text1, text2, text3, zone_id in zone_names
        )

    def header(self, *, pragma_once: bool = False) -> str:
        kind = "hpp" if pragma_once else "cpp"
        return self.header_config.source_header(kind, pragma_once=pragma_once, tool="generate_time_zone_data.py")

    def write(self, name: str, content: str) -> None:
        path = self.output_dir / name
        content = self.format_cpp(content, path)
        if self.file_update.write_if_changed(path, content):
            self.generated_files.append(path)

    @staticmethod
    def format_cpp(content: str, path: Path) -> str:
        if path.suffix not in {".cpp", ".hpp"}:
            return content
        try:
            result = subprocess.run(
                ["clang-format", f"--assume-filename={path}"],
                input=content,
                text=True,
                capture_output=True,
                check=True,
            )
        except (OSError, subprocess.CalledProcessError) as error:
            raise UtilityError(f"Failed to format generated C++ file {path.name}: {error}") from error
        return result.stdout
