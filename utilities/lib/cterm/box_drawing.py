# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Shared Unicode box-drawing character analysis for cterm generators."""

from __future__ import annotations

import unicodedata
from dataclasses import asdict, dataclass
from enum import IntEnum

BOX_FRAME_CHARACTER_DATA = """\
┌┬┐├┼┤└┴┘
┏┳┓┣╋┫┗┻┛
┍┯┑┝┿┥┕┷┙
┎┰┒┠╂┨┖┸┚
╒╤╕╞╪╡╘╧╛
╔╦╗╠╬╣╚╩╝
╓╥╖╟╫╢╙╨╜
╼╾╽╿╱╲╳
 ┮┱┲┭
┡┽╀╁┾┩
┟╆╈╇╅┧
┞╄╉╊╃┦
┢┶┹┺┵┪
╴╵╶╷╭╮
╸╹╺╻╰╯
│┃┊┋┆┇
─━┈┉┄┅
╎╌╏╍║═︎
∙▪▫
"""
POSITION_NAMES = ("right", "down", "left", "up", "slash", "backslash", "center")
POSITION_WEIGHTS = {
    "right": 4,
    "down": 4,
    "left": 4,
    "up": 4,
    "slash": 5,
    "backslash": 5,
    "center": 6,
}
MINIMUM_IMPROVEMENT = 100


class StrokeType(IntEnum):
    NONE = 0
    LIGHT_QUADRUPLE_DASH = 1
    HEAVY_QUADRUPLE_DASH = 2
    LIGHT_TRIPLE_DASH = 3
    HEAVY_TRIPLE_DASH = 4
    LIGHT_DOUBLE_DASH = 5
    HEAVY_DOUBLE_DASH = 6
    LIGHT = 7
    DOUBLE = 8
    HEAVY = 9


STYLE_TOKENS = {
    "LIGHT": StrokeType.LIGHT,
    "HEAVY": StrokeType.HEAVY,
    "DOUBLE": StrokeType.DOUBLE,
    "SINGLE": StrokeType.LIGHT,
}
GROUPS = {
    "UP": ("up",),
    "DOWN": ("down",),
    "LEFT": ("left",),
    "RIGHT": ("right",),
    "VERTICAL": ("up", "down"),
    "HORIZONTAL": ("left", "right"),
    "LEFT DOWN": ("left", "down"),
    "RIGHT DOWN": ("right", "down"),
    "LEFT UP": ("left", "up"),
    "RIGHT UP": ("right", "up"),
    "DOWN HORIZONTAL": ("down", "left", "right"),
    "UP HORIZONTAL": ("up", "left", "right"),
    "RIGHT VERTICAL": ("right", "up", "down"),
    "LEFT VERTICAL": ("left", "up", "down"),
}


@dataclass(frozen=True)
class Attributes:
    right: StrokeType = StrokeType.NONE
    down: StrokeType = StrokeType.NONE
    left: StrokeType = StrokeType.NONE
    up: StrokeType = StrokeType.NONE
    slash: StrokeType = StrokeType.NONE
    backslash: StrokeType = StrokeType.NONE
    center: StrokeType = StrokeType.NONE


def all_characters() -> list[str]:
    """Return all unique source characters used by the box-drawing generators."""
    result: list[str] = []
    for character in BOX_FRAME_CHARACTER_DATA:
        if character == "\n" or 0xFE00 <= ord(character) <= 0xFE0F:
            continue
        if character not in result:
            result.append(character)
    return result


def apply_positions(
    attributes: Attributes,
    positions: tuple[str, ...],
    stroke_type: StrokeType,
    *,
    include_center: bool = True,
) -> Attributes:
    """Set the given positions to a stroke type."""
    values = asdict(attributes)
    for position in positions:
        values[position] = stroke_type
    if include_center and stroke_type > values["center"]:
        values["center"] = stroke_type
    return Attributes(**values)


def parse_group(text: str) -> tuple[str, ...]:
    """Parse a Unicode box-drawing group name."""
    if text not in GROUPS:
        raise ValueError(f"Unsupported box-drawing group: {text!r}")
    return GROUPS[text]


def extract_style(segment: str, inherited_style: StrokeType | None) -> tuple[StrokeType, str]:
    """Extract the stroke style and position group from one Unicode name segment."""
    segment = segment.replace(" SINGLE", " LIGHT")
    dash_variant: str | None = None
    for marker, variant in (
        (" QUADRUPLE DASH", "quadruple"),
        (" TRIPLE DASH", "triple"),
        (" DOUBLE DASH", "double"),
    ):
        if marker in segment:
            segment = segment.replace(marker, "")
            dash_variant = variant
            break
    words = segment.split()
    if words and words[0] in STYLE_TOKENS:
        stroke_type = STYLE_TOKENS[words[0]]
        group_text = " ".join(words[1:])
    elif words and words[-1] in STYLE_TOKENS:
        stroke_type = STYLE_TOKENS[words[-1]]
        group_text = " ".join(words[:-1])
    else:
        if inherited_style is None:
            raise ValueError(f"Cannot infer style for segment: {segment!r}")
        stroke_type = inherited_style
        group_text = segment
    if dash_variant == "quadruple":
        stroke_type = (
            StrokeType.HEAVY_QUADRUPLE_DASH if stroke_type == StrokeType.HEAVY else StrokeType.LIGHT_QUADRUPLE_DASH
        )
    elif dash_variant == "triple":
        stroke_type = StrokeType.HEAVY_TRIPLE_DASH if stroke_type == StrokeType.HEAVY else StrokeType.LIGHT_TRIPLE_DASH
    elif dash_variant == "double":
        stroke_type = StrokeType.HEAVY_DOUBLE_DASH if stroke_type == StrokeType.HEAVY else StrokeType.LIGHT_DOUBLE_DASH
    return stroke_type, group_text


def attributes_from_name(character: str) -> Attributes:
    """Infer drawing attributes from a Unicode box-drawing character name."""
    if character == " ":
        return Attributes()
    if character == "∙":
        return Attributes(center=StrokeType.LIGHT)
    if character == "▪":
        return Attributes(center=StrokeType.HEAVY)
    if character == "▫":
        return Attributes(center=StrokeType.DOUBLE)

    unicode_name = unicodedata.name(character)
    if not unicode_name.startswith("BOX DRAWINGS "):
        raise ValueError(f"Unsupported character: {character!r}")
    box_name = unicode_name.removeprefix("BOX DRAWINGS ")

    if box_name.startswith("LIGHT ARC "):
        attributes = Attributes()
        for segment in box_name.removeprefix("LIGHT ARC ").split(" AND "):
            attributes = apply_positions(attributes, parse_group(segment), StrokeType.LIGHT, include_center=False)
        return attributes
    if box_name == "LIGHT DIAGONAL UPPER RIGHT TO LOWER LEFT":
        return Attributes(slash=StrokeType.LIGHT, center=StrokeType.LIGHT)
    if box_name == "LIGHT DIAGONAL UPPER LEFT TO LOWER RIGHT":
        return Attributes(backslash=StrokeType.LIGHT, center=StrokeType.LIGHT)
    if box_name == "LIGHT DIAGONAL CROSS":
        return Attributes(slash=StrokeType.LIGHT, backslash=StrokeType.LIGHT, center=StrokeType.LIGHT)

    attributes = Attributes()
    inherited_style: StrokeType | None = None
    for segment in box_name.split(" AND "):
        stroke_type, group_text = extract_style(segment, inherited_style)
        attributes = apply_positions(attributes, parse_group(group_text), stroke_type)
        inherited_style = stroke_type
    return attributes


def build_character_attributes(characters: list[str]) -> dict[str, Attributes]:
    """Build an attribute map for all supported characters."""
    return {character: attributes_from_name(character) for character in characters}


def build_attributes_to_character(character_attributes: dict[str, Attributes]) -> dict[Attributes, str]:
    """Build a reverse map from attributes to the first matching character."""
    return {attributes: character for character, attributes in character_attributes.items()}


def combine_attributes(current: Attributes, overlay: Attributes) -> Attributes:
    """Combine two box-drawing attribute sets by preserving the strongest stroke per position."""
    return Attributes(
        **{position: max(getattr(current, position), getattr(overlay, position)) for position in POSITION_NAMES}
    )


def has_lines(attributes: Attributes) -> bool:
    """Test if an attribute set has any actual line segment."""
    return any(getattr(attributes, position) != StrokeType.NONE for position in POSITION_NAMES[:-1])


def is_center_only(attributes: Attributes) -> bool:
    """Test if an attribute set only marks the center cell."""
    return attributes.center != StrokeType.NONE and not has_lines(attributes)


def score_candidate(ideal: Attributes, candidate: Attributes) -> int:
    """Score a candidate character for an ideal attribute set."""
    missing_score = 0
    distance_score = 0
    extra_score = 0
    for position in POSITION_NAMES:
        ideal_stroke = getattr(ideal, position)
        candidate_stroke = getattr(candidate, position)
        weight = POSITION_WEIGHTS[position]
        if ideal_stroke != StrokeType.NONE and candidate_stroke == StrokeType.NONE:
            missing_score += weight
        elif ideal_stroke == StrokeType.NONE and candidate_stroke != StrokeType.NONE:
            extra_score += weight * (2 + int(candidate_stroke))
        elif ideal_stroke != StrokeType.NONE and candidate_stroke != StrokeType.NONE:
            distance_score += weight * abs(int(ideal_stroke) - int(candidate_stroke))
    return missing_score * 10_000 + distance_score * 100 + extra_score


def choose_best_character(
    ideal: Attributes,
    character_attributes: dict[str, Attributes],
    attributes_to_character: dict[Attributes, str],
    characters: list[str],
) -> tuple[str, str]:
    """Choose the best supported character for an ideal attribute set."""
    exact_match = attributes_to_character.get(ideal)
    if exact_match is not None:
        return exact_match, "exact"
    best_character = min(characters, key=lambda character: score_candidate(ideal, character_attributes[character]))
    return best_character, "best"


def choose_combination_result(
    current: str,
    overlay: str,
    character_attributes: dict[str, Attributes],
    attributes_to_character: dict[Attributes, str],
    characters: list[str],
) -> tuple[str, str]:
    """Choose the result for overlaying one box-drawing character on another."""
    current_attributes = character_attributes[current]
    overlay_attributes = character_attributes[overlay]
    if is_center_only(current_attributes) and has_lines(overlay_attributes):
        return overlay, "center-overwrite"
    if has_lines(current_attributes) and is_center_only(overlay_attributes):
        return current, "center-overwrite"

    ideal = combine_attributes(current_attributes, overlay_attributes)
    exact_match = attributes_to_character.get(ideal)
    if exact_match is not None:
        return exact_match, "exact"

    best_character = min(characters, key=lambda character: score_candidate(ideal, character_attributes[character]))
    best_score = score_candidate(ideal, character_attributes[best_character])
    overlay_score = score_candidate(ideal, overlay_attributes)
    if best_score + MINIMUM_IMPROVEMENT < overlay_score:
        return best_character, "best"
    return overlay, "overlay"
