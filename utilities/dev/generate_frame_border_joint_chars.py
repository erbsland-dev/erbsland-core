#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Generate the frame-border joint character lookup table."""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

from lib.copyright import HeaderConfig
from lib.cterm.box_drawing import (
    Attributes,
    StrokeType,
    all_characters,
    apply_positions,
    build_attributes_to_character,
    build_character_attributes,
    choose_best_character,
)
from lib.cterm.cpp_render import cpp_byte_string_literal, cpp_u32_string_literal, generated_cpp_file
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.utility import UtilityApp

ROUNDED_CORNERS = {
    frozenset(("right", "down")): "╭",
    frozenset(("left", "down")): "╮",
    frozenset(("right", "up")): "╰",
    frozenset(("left", "up")): "╯",
}
DIRECTIONS = ("right", "down", "left", "up")


@dataclass(frozen=True)
class JointStyle:
    name: str
    stroke_type: StrokeType
    rounded: bool = False


JOINT_STYLES = [
    JointStyle("None", StrokeType.NONE),
    JointStyle("Light", StrokeType.LIGHT),
    JointStyle("Heavy", StrokeType.HEAVY),
    JointStyle("Double", StrokeType.DOUBLE),
    JointStyle("LightRounded", StrokeType.LIGHT, rounded=True),
]


class GenerateFrameBorderJointCharsApp(UtilityApp):
    """Generate compact lookup data for frame-border joint characters."""

    description = "Generate the cterm frame-border joint character lookup table."

    TOOL_NAME = "generate_frame_border_joint_chars.py"

    def __init__(self) -> None:
        super().__init__()
        self.check = False
        self.output_path = Path()
        self.file_update = FileUpdate(self.print_verbose)

    def add_command_line_args(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("--check", action="store_true", help="Verify that the generated file is up to date.")

    def handle_command_line_args(self, args: argparse.Namespace) -> None:
        self.check = args.check
        self.output_path = self.project_directory / "src" / "erbsland" / "cterm" / "impl" / "FrameBorder_data.hpp"

    @staticmethod
    def joint_character(
        east: JointStyle,
        south: JointStyle,
        west: JointStyle,
        north: JointStyle,
        character_attributes: dict[str, Attributes],
        attributes_to_character: dict[Attributes, str],
        characters: list[str],
    ) -> tuple[str, str]:
        """Resolve one joint character."""
        styles = (east, south, west, north)
        active_positions = [direction for direction, style in zip(DIRECTIONS, styles, strict=True) if style.stroke_type]
        if (
            len(active_positions) == 2
            and all(style.rounded for style in styles if style.stroke_type)
            and frozenset(active_positions) in ROUNDED_CORNERS
        ):
            return ROUNDED_CORNERS[frozenset(active_positions)], "rounded"

        ideal = Attributes()
        for direction, style in zip(DIRECTIONS, styles, strict=True):
            if style.stroke_type == StrokeType.NONE:
                continue
            ideal = apply_positions(ideal, (direction,), style.stroke_type)
        return choose_best_character(ideal, character_attributes, attributes_to_character, characters)

    def generate_table(self) -> tuple[list[str], list[int], Counter[str]]:
        """Generate the compact character set and byte index table."""
        characters = all_characters()
        character_attributes = build_character_attributes(characters)
        attributes_to_character = build_attributes_to_character(character_attributes)
        result_characters: list[str] = []
        result_indexes: list[int] = []
        selection_counts: Counter[str] = Counter()

        for east in JOINT_STYLES:
            for south in JOINT_STYLES:
                for west in JOINT_STYLES:
                    for north in JOINT_STYLES:
                        character, selection = self.joint_character(
                            east=east,
                            south=south,
                            west=west,
                            north=north,
                            character_attributes=character_attributes,
                            attributes_to_character=attributes_to_character,
                            characters=characters,
                        )
                        if character not in result_characters:
                            result_characters.append(character)
                        result_indexes.append(result_characters.index(character))
                        selection_counts[selection] += 1
        return result_characters, result_indexes, selection_counts

    def render_output(
        self,
        result_characters: list[str],
        result_indexes: list[int],
        selection_counts: Counter[str],
        header_config: HeaderConfig,
    ) -> str:
        """Render the generated C++ data header."""
        style_count = len(JOINT_STYLES)
        table_size = len(result_indexes)
        summary_lines = [
            (
                f"Generated from {style_count} style classes and {table_size} directed style joints: "
                f"{selection_counts['exact']} exact, "
                f"{selection_counts['best']} weighted matches, "
                f"{selection_counts['rounded']} rounded corners."
            ),
            "Only passive lookup data is generated; the implementation lives in FrameBorder.cpp.",
        ]
        body_lines = [
            "#include <cstddef>",
            "#include <string_view>",
            "",
            "namespace erbsland::cterm::impl {",
            "",
            f"inline constexpr auto cFrameBorderJointStyleCount = std::size_t{{{style_count}}};",
            f"inline constexpr auto cFrameBorderJointTableSize = std::size_t{{{table_size}}};",
            "inline constexpr auto cFrameBorderJointCharacters =",
        ]
        character_literals = cpp_u32_string_literal("".join(result_characters))
        for index, line in enumerate(character_literals):
            suffix = ";" if index == len(character_literals) - 1 else ""
            body_lines.append(f"    {line}{suffix}")
        body_lines.append("inline constexpr auto cFrameBorderJointCharacterIndexes = std::string_view{")
        for line in cpp_byte_string_literal(result_indexes):
            body_lines.append(f"    {line}")
        body_lines.extend(
            [
                "    , cFrameBorderJointTableSize};",
                "",
                "}",
                "",
            ]
        )
        return generated_cpp_file(header_config, "hpp", self.TOOL_NAME, summary_lines, body_lines)

    def run(self, argv=None) -> None:
        super().run(argv)
        header_config = HeaderConfig.read(self.config_file_path())
        result_characters, result_indexes, selection_counts = self.generate_table()
        output = self.render_output(result_characters, result_indexes, selection_counts, header_config)
        if self.check:
            if FileUpdate.has_changed(self.output_path, output):
                raise UtilityError("The generated frame border joint character data is out of date.")
            print("Frame border joint character data is up to date.")
            return
        if self.file_update.write_if_changed(self.output_path, output):
            print(f"Updated {self.output_path.relative_to(self.project_directory)}")
        else:
            print("Unchanged frame border joint character data.")


def main() -> None:
    raise SystemExit(GenerateFrameBorderJointCharsApp().main())


if __name__ == "__main__":
    main()
