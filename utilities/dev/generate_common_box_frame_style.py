#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Generate the common box-frame character combination data."""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path

from lib.copyright import HeaderConfig
from lib.cterm.box_drawing import (
    Attributes,
    POSITION_NAMES,
    POSITION_WEIGHTS,
    all_characters,
    build_attributes_to_character,
    build_character_attributes,
    combine_attributes,
    choose_combination_result,
    has_lines,
    is_center_only,
    score_candidate,
)
from lib.cterm.cpp_render import generated_cpp_file, render_data_function
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.utility import UtilityApp


class GenerateCommonBoxFrameStyleApp(UtilityApp):
    """Generate compact lookup data for common cterm box-frame character combinations."""

    description = "Generate the cterm common box-frame character combination data."

    TOOL_NAME = "generate_common_box_frame_style.py"
    ATTRIBUTE_SHIFT = 4
    BOX_DRAWING_START = 0x2500
    BOX_DRAWING_END = 0x257F
    UNSUPPORTED_INDEX = 0xFF
    MINIMUM_IMPROVEMENT = 100

    def __init__(self) -> None:
        super().__init__()
        self.check = False
        self.output_path = Path()
        self.file_update = FileUpdate(self.print_verbose)

    def add_command_line_args(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("--check", action="store_true", help="Verify that the generated file is up to date.")

    def handle_command_line_args(self, args: argparse.Namespace) -> None:
        self.check = args.check
        self.output_path = (
            self.project_directory / "src" / "erbsland" / "cterm" / "impl" / "CommonBoxFrameCombinationStyle_data.cpp"
        )

    @classmethod
    def is_box_drawing(cls, character: str) -> bool:
        """Test if a character is in the Unicode box-drawing block."""
        return cls.BOX_DRAWING_START <= ord(character) <= cls.BOX_DRAWING_END

    @classmethod
    def pack_attributes(cls, attributes: Attributes) -> int:
        """Pack attributes into a compact 4-bit-per-position integer."""
        value = 0
        for index, position in enumerate(POSITION_NAMES):
            value |= int(getattr(attributes, position)) << (index * cls.ATTRIBUTE_SHIFT)
        return value

    @classmethod
    def compact_result(
        cls,
        current: str,
        overlay: str,
        character_attributes: dict[str, Attributes],
        attributes_to_character: dict[Attributes, str],
        characters: list[str],
    ) -> str:
        """Return the result produced by the compact runtime algorithm."""
        current_attributes = character_attributes[current]
        overlay_attributes = character_attributes[overlay]
        if is_center_only(current_attributes) and has_lines(overlay_attributes):
            return overlay
        if has_lines(current_attributes) and is_center_only(overlay_attributes):
            return current

        ideal = combine_attributes(current_attributes, overlay_attributes)
        exact_match = attributes_to_character.get(ideal)
        if exact_match is not None:
            return exact_match

        best_character = min(characters, key=lambda character: score_candidate(ideal, character_attributes[character]))
        best_score = score_candidate(ideal, character_attributes[best_character])
        overlay_score = score_candidate(ideal, overlay_attributes)
        if best_score + cls.MINIMUM_IMPROVEMENT < overlay_score:
            return best_character
        return overlay

    def verify_compact_algorithm(self, characters: list[str]) -> Counter[str]:
        """Verify the compact runtime algorithm against the reference generator rules."""
        character_attributes = build_character_attributes(characters)
        attributes_to_character = build_attributes_to_character(character_attributes)
        selection_counts: Counter[str] = Counter()
        for current in characters:
            for overlay in characters:
                reference_result, selection = choose_combination_result(
                    current=current,
                    overlay=overlay,
                    character_attributes=character_attributes,
                    attributes_to_character=attributes_to_character,
                    characters=characters,
                )
                compact = self.compact_result(
                    current,
                    overlay,
                    character_attributes,
                    attributes_to_character,
                    characters,
                )
                if compact != reference_result:
                    raise UtilityError(
                        f"Compact algorithm mismatch for {current!r} + {overlay!r}: "
                        f"{compact!r} != {reference_result!r}"
                    )
                selection_counts[selection] += 1
        return selection_counts

    def special_index_entries(self, characters: list[str]) -> list[tuple[str, int]]:
        """Build the special non-box code point lookup entries."""
        return [(character, index) for index, character in enumerate(characters) if not self.is_box_drawing(character)]

    def render_output(
        self,
        characters: list[str],
        selection_counts: Counter[str],
        header_config: HeaderConfig,
    ) -> str:
        """Render the generated C++ source file."""
        character_attributes = build_character_attributes(characters)
        attributes_to_character = build_attributes_to_character(character_attributes)
        packed_attributes = [self.pack_attributes(character_attributes[character]) for character in characters]
        exact_entries = sorted(
            (self.pack_attributes(attributes), ord(character))
            for attributes, character in attributes_to_character.items()
        )
        box_offset_to_character_index = [self.UNSUPPORTED_INDEX] * (self.BOX_DRAWING_END - self.BOX_DRAWING_START + 1)
        for index, character in enumerate(characters):
            if self.is_box_drawing(character):
                box_offset_to_character_index[ord(character) - self.BOX_DRAWING_START] = index

        special_entries = self.special_index_entries(characters)
        summary_lines = [
            (
                f"Generated from {sum(selection_counts.values())} directed combinations: "
                f"{selection_counts['exact']} exact, "
                f"{selection_counts['best']} weighted matches, "
                f"{selection_counts['overlay']} overlay fallbacks, "
                f"{selection_counts['center-overwrite']} center overwrites."
            ),
            (
                f"Compact runtime data: {len(characters)} characters, {len(exact_entries)} exact attribute entries, "
                f"{len(box_offset_to_character_index)} box lookup entries."
            ),
        ]
        body_lines = [
            '#include "CommonBoxFrameCombinationStyle.hpp"',
            "",
            "#include <array>",
            "",
            "namespace erbsland::cterm {",
            "",
        ]
        body_lines.extend(
            render_data_function(
                "boxOffsetToCharacterIndex",
                "uint8_t",
                box_offset_to_character_index,
                entries_per_line=12,
                width=2,
            )
        )
        body_lines.extend(
            render_data_function(
                "characters",
                "CommonBoxFrameCombinationStyle::CodePointData",
                [ord(character) for character in characters],
                entries_per_line=8,
                width=4,
            )
        )
        body_lines.extend(
            render_data_function(
                "attributes",
                "CommonBoxFrameCombinationStyle::AttributeData",
                packed_attributes,
                entries_per_line=8,
                width=7,
            )
        )
        body_lines.extend(
            render_data_function(
                "exactAttributes",
                "CommonBoxFrameCombinationStyle::AttributeData",
                [entry[0] for entry in exact_entries],
                entries_per_line=8,
                width=7,
            )
        )
        body_lines.extend(
            render_data_function(
                "exactCharacters",
                "CommonBoxFrameCombinationStyle::CodePointData",
                [entry[1] for entry in exact_entries],
                entries_per_line=8,
                width=4,
            )
        )
        body_lines.extend(
            render_data_function(
                "positionWeights",
                "uint8_t",
                [POSITION_WEIGHTS[position] for position in POSITION_NAMES],
                entries_per_line=len(POSITION_NAMES),
                width=2,
            )
        )
        body_lines.extend(
            render_data_function(
                "specialCodePoints",
                "CommonBoxFrameCombinationStyle::CodePointData",
                [ord(character) for character, _ in special_entries],
                entries_per_line=8,
                width=4,
            )
        )
        body_lines.extend(
            render_data_function(
                "specialCharacterIndexes",
                "uint8_t",
                [index for _, index in special_entries],
                entries_per_line=8,
                width=2,
            )
        )
        body_lines.extend(["}", ""])
        return generated_cpp_file(header_config, "cpp", self.TOOL_NAME, summary_lines, body_lines)

    def run(self, argv=None) -> None:
        super().run(argv)
        header_config = HeaderConfig.read(self.config_file_path())
        characters = all_characters()
        selection_counts = self.verify_compact_algorithm(characters)
        output = self.render_output(characters, selection_counts, header_config)
        if self.check:
            if FileUpdate.has_changed(self.output_path, output):
                raise UtilityError("The generated common box frame style data file is out of date.")
            print("Common box frame style data is up to date.")
            return
        if self.file_update.write_if_changed(self.output_path, output):
            print(f"Updated {self.output_path.relative_to(self.project_directory)}")
        else:
            print("Unchanged common box frame style data.")


def main() -> None:
    raise SystemExit(GenerateCommonBoxFrameStyleApp().main())


if __name__ == "__main__":
    main()
