#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Generate the compact HTTP client Public Suffix List lookup table."""

from __future__ import annotations

import argparse
import heapq
import re
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import TypeAlias

import idna

from lib.utility import UtilityApp

HuffmanNode: TypeAlias = int | tuple["HuffmanNode", "HuffmanNode"]


@dataclass(frozen=True)
class EncodedPublicSuffixData:
    """The generated compressed Public Suffix List payload."""

    rules: tuple[tuple[str, int], ...]
    encoded_rules: bytes
    block_bit_offsets: tuple[int, ...]
    prefix_tree: bytes
    content_tree: bytes
    maximum_rule_length: int
    maximum_rule_label_count: int
    original_rule_text_byte_count: int

    @property
    def encoded_storage_byte_count(self) -> int:
        """Return the complete persistent byte count of the encoded tables."""
        return (
            len(self.encoded_rules) + len(self.block_bit_offsets) * 4 + len(self.prefix_tree) + len(self.content_tree)
        )


class BitWriter:
    """Write an MSB-first bitstream."""

    def __init__(self) -> None:
        self.data = bytearray()
        self.bit_length = 0

    def write(self, code: int, length: int) -> None:
        """Append one code to the stream."""
        for shift in range(length - 1, -1, -1):
            if self.bit_length % 8 == 0:
                self.data.append(0)
            if (code >> shift) & 1:
                self.data[-1] |= 1 << (7 - self.bit_length % 8)
            self.bit_length += 1


class BitReader:
    """Read an MSB-first bitstream for generator verification."""

    def __init__(self, data: bytes, bit_position: int) -> None:
        self.data = data
        self.bit_position = bit_position

    def read(self) -> int:
        """Read one bit."""
        byte = self.data[self.bit_position // 8]
        result = (byte >> (7 - self.bit_position % 8)) & 1
        self.bit_position += 1
        return result


class GeneratePublicSuffixDataApp(UtilityApp):
    """Generate sorted compressed IDNA ASCII PSL data for the HTTP client."""

    description = "Generate the complete ICANN and private Public Suffix List lookup table."

    block_size = 32
    exact_token = 0
    wildcard_token = 1
    exception_token = 2
    first_character_token = 3
    characters = "-.0123456789abcdefghijklmnopqrstuvwxyz"

    def __init__(self) -> None:
        super().__init__()
        self.check = False
        self.input_path = Path()
        self.output_path = Path()

    def add_command_line_args(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("--check", action="store_true", help="Verify that the generated table is current.")

    def handle_command_line_args(self, args: argparse.Namespace) -> None:
        self.check = args.check
        self.input_path = self.project_directory / "utilities" / "data" / "public_suffix" / "public_suffix_list.dat"
        self.output_path = (
            self.project_directory
            / "src"
            / "erbsland"
            / "network"
            / "impl"
            / "http"
            / "cookie"
            / "PublicSuffixData.cpp"
        )

    @staticmethod
    def ascii_rule(rule: str) -> str:
        """Convert one Unicode PSL rule to canonical IDNA2008 ASCII."""
        return ".".join(idna.encode(label, uts46=True).decode("ascii") for label in rule.split("."))

    def parse(self) -> tuple[str, list[str], list[str], list[str]]:
        """Parse the pinned source into exact, wildcard, and exception rules."""
        source = self.input_path.read_text(encoding="utf-8")
        version_match = re.search(r"^// VERSION: (.+)$", source, re.MULTILINE)
        if version_match is None:
            raise RuntimeError("The Public Suffix List has no VERSION header.")
        exact: set[str] = set()
        wildcard: set[str] = set()
        exception: set[str] = set()
        for raw_line in source.splitlines():
            line = raw_line.strip()
            if not line or line.startswith("//"):
                continue
            target = exact
            if line.startswith("! "):
                raise RuntimeError(f"Malformed exception rule: {line}")
            if line.startswith("!"):
                target = exception
                line = line[1:]
            elif line.startswith("*."):
                target = wildcard
                line = line[2:]
            target.add(self.ascii_rule(line).lower())
        return version_match.group(1), sorted(exact), sorted(wildcard), sorted(exception)

    @staticmethod
    def reverse_labels(rule: str) -> str:
        """Reverse the labels in one rule while preserving each label's spelling."""
        return ".".join(reversed(rule.split(".")))

    @staticmethod
    def common_prefix_length(left: str, right: str) -> int:
        """Return the common byte-prefix length of two ASCII rules."""
        result = 0
        while result < min(len(left), len(right)) and left[result] == right[result]:
            result += 1
        return result

    @staticmethod
    def build_huffman_tree(frequencies: Counter[int]) -> HuffmanNode:
        """Build a deterministic Huffman tree using token order for tie-breaking."""
        heap: list[tuple[int, int, int, HuffmanNode]] = []
        serial = 0
        for token, frequency in sorted(frequencies.items()):
            heapq.heappush(heap, (frequency, token, serial, token))
            serial += 1
        if len(heap) < 2:
            raise RuntimeError("A Huffman alphabet must contain at least two tokens.")
        while len(heap) > 1:
            left_frequency, left_token, _, left = heapq.heappop(heap)
            right_frequency, right_token, _, right = heapq.heappop(heap)
            minimum_token = min(left_token, right_token)
            heapq.heappush(
                heap,
                (left_frequency + right_frequency, minimum_token, serial, (left, right)),
            )
            serial += 1
        return heap[0][3]

    @staticmethod
    def huffman_codes(tree: HuffmanNode) -> dict[int, tuple[int, int]]:
        """Create the bit code and length for every leaf in a Huffman tree."""
        result: dict[int, tuple[int, int]] = {}

        def visit(node: HuffmanNode, code: int, length: int) -> None:
            if isinstance(node, int):
                result[node] = (code, length)
                return
            visit(node[0], code << 1, length + 1)
            visit(node[1], (code << 1) | 1, length + 1)

        visit(tree, 0, 0)
        return result

    @staticmethod
    def flatten_huffman_tree(tree: HuffmanNode, leaf_mask: int) -> tuple[int, ...]:
        """Flatten branch pairs and encode leaf values with the supplied marker bit."""
        nodes: list[HuffmanNode] = []

        def add_node(node: HuffmanNode) -> int:
            if isinstance(node, int):
                raise RuntimeError("The Huffman root must be a branch.")
            index = len(nodes)
            nodes.append(node)
            for child in node:
                if not isinstance(child, int):
                    add_node(child)
            return index

        add_node(tree)
        indices = {id(node): index for index, node in enumerate(nodes)}
        result: list[int] = []
        for node in nodes:
            if isinstance(node, int):
                raise RuntimeError("Unexpected Huffman leaf in branch table.")
            for child in node:
                if isinstance(child, int):
                    if child >= leaf_mask:
                        raise RuntimeError("Huffman leaf token does not fit into the flattened tree.")
                    result.append(leaf_mask | child)
                else:
                    child_index = indices[id(child)]
                    if child_index >= leaf_mask:
                        raise RuntimeError("Huffman branch index does not fit into the flattened tree.")
                    result.append(child_index)
        return tuple(result)

    @staticmethod
    def decode_huffman_token(reader: BitReader, tree: tuple[int, ...] | bytes, leaf_mask: int) -> int:
        """Decode one token from a flattened Huffman tree."""
        node = 0
        while True:
            entry = tree[node * 2 + reader.read()]
            if entry & leaf_mask:
                return entry & (leaf_mask - 1)
            node = entry

    def encode(self, exact: list[str], wildcard: list[str], exception: list[str]) -> EncodedPublicSuffixData:
        """Encode all rule families into the reversed-label Huffman stream."""
        typed_rules = [(rule, self.exact_token) for rule in exact]
        typed_rules += [(rule, self.wildcard_token) for rule in wildcard]
        typed_rules += [(rule, self.exception_token) for rule in exception]
        transformed_rules = tuple(sorted((self.reverse_labels(rule), token) for rule, token in typed_rules))
        if len({rule for rule, _ in transformed_rules}) != len(transformed_rules):
            raise RuntimeError("Public Suffix List rule families overlap after transformation.")
        allowed_characters = set(self.characters)
        for rule, _ in transformed_rules:
            if not set(rule) <= allowed_characters:
                raise RuntimeError(f"Unsupported character in transformed rule: {rule}")

        character_tokens = {
            character: self.first_character_token + index for index, character in enumerate(self.characters)
        }
        records: list[tuple[int, tuple[int, ...]]] = []
        prefix_frequencies: Counter[int] = Counter()
        content_frequencies: Counter[int] = Counter()
        previous = ""
        for index, (rule, terminal_token) in enumerate(transformed_rules):
            prefix_length = 0
            if index % self.block_size != 0:
                prefix_length = self.common_prefix_length(previous, rule)
            content = tuple(character_tokens[character] for character in rule[prefix_length:]) + (terminal_token,)
            records.append((prefix_length, content))
            prefix_frequencies[prefix_length] += 1
            content_frequencies.update(content)
            previous = rule

        prefix_tree = self.build_huffman_tree(prefix_frequencies)
        content_tree = self.build_huffman_tree(content_frequencies)
        prefix_codes = self.huffman_codes(prefix_tree)
        content_codes = self.huffman_codes(content_tree)
        flat_prefix_tree = bytes(self.flatten_huffman_tree(prefix_tree, 0x80))
        flat_content_tree = bytes(self.flatten_huffman_tree(content_tree, 0x80))

        writer = BitWriter()
        block_bit_offsets: list[int] = []
        for index, (prefix_length, content) in enumerate(records):
            if index % self.block_size == 0:
                block_bit_offsets.append(writer.bit_length)
            writer.write(*prefix_codes[prefix_length])
            for token in content:
                writer.write(*content_codes[token])
        block_bit_offsets.append(writer.bit_length)
        if writer.bit_length > 0xFFFFFFFF:
            raise RuntimeError("Encoded Public Suffix List exceeds the 32-bit bit-offset range.")

        result = EncodedPublicSuffixData(
            rules=transformed_rules,
            encoded_rules=bytes(writer.data),
            block_bit_offsets=tuple(block_bit_offsets),
            prefix_tree=flat_prefix_tree,
            content_tree=flat_content_tree,
            maximum_rule_length=max(len(rule) for rule, _ in transformed_rules),
            maximum_rule_label_count=max(rule.count(".") + 1 for rule, _ in transformed_rules),
            original_rule_text_byte_count=sum(len(rule) + 1 for rule, _ in typed_rules),
        )
        self.validate_decoding(result)
        return result

    def validate_decoding(self, data: EncodedPublicSuffixData) -> None:
        """Decode the complete payload and require an exact rule round trip."""
        decoded_rules: list[tuple[str, int]] = []
        token_characters = {
            self.first_character_token + index: character for index, character in enumerate(self.characters)
        }
        prefix_tree = tuple(data.prefix_tree)
        content_tree = tuple(data.content_tree)
        for block_index, bit_offset in enumerate(data.block_bit_offsets[:-1]):
            reader = BitReader(data.encoded_rules, bit_offset)
            previous = ""
            first_rule = block_index * self.block_size
            last_rule = min(first_rule + self.block_size, len(data.rules))
            for _ in range(first_rule, last_rule):
                prefix_length = self.decode_huffman_token(reader, prefix_tree, 0x80)
                if prefix_length > len(previous):
                    raise RuntimeError("Encoded rule prefix exceeds the previous rule.")
                rule = previous[:prefix_length]
                while True:
                    token = self.decode_huffman_token(reader, content_tree, 0x80)
                    if token <= self.exception_token:
                        break
                    try:
                        rule += token_characters[token]
                    except KeyError as error:
                        raise RuntimeError(f"Invalid encoded content token: {token}") from error
                decoded_rules.append((rule, token))
                previous = rule
        if tuple(decoded_rules) != data.rules:
            raise RuntimeError("Encoded Public Suffix List does not round trip.")

    @staticmethod
    def render_array(type_name: str, name: str, values: tuple[int, ...] | bytes, width: int) -> str:
        """Render a generated C++ integer array."""
        lines = []
        for start in range(0, len(values), 16):
            entries = values[start : start + 16]
            if width == 2:
                rendered = ", ".join(f"0x{value:04x}U" for value in entries)
            elif width == 1:
                rendered = ", ".join(f"0x{value:02x}U" for value in entries)
            else:
                rendered = ", ".join(f"{value}U" for value in entries)
            lines.append(f"        {rendered},")
        return (
            f"    static constexpr auto {name} = std::array<{type_name}, {len(values)}U>{{{{\n"
            + "\n".join(lines)
            + "\n    }};\n"
        )

    def render(self, version: str, data: EncodedPublicSuffixData) -> str:
        """Render the generated implementation."""
        exact_count = sum(token == self.exact_token for _, token in data.rules)
        wildcard_count = sum(token == self.wildcard_token for _, token in data.rules)
        exception_count = sum(token == self.exception_token for _, token in data.rules)
        legacy_32_size = data.original_rule_text_byte_count + len(data.rules) * 8
        arrays = "\n".join(
            [
                self.render_array("std::uint8_t", "cEncodedRules", data.encoded_rules, 1),
                self.render_array("std::uint32_t", "cBlockBitOffsets", data.block_bit_offsets, 4),
                self.render_array("std::uint8_t", "cPrefixTree", data.prefix_tree, 1),
                self.render_array("std::uint8_t", "cContentTree", data.content_tree, 1),
            ]
        )
        return f"""// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
// !!! THIS IS A GENERATED FILE - DO NOT EDIT MANUALLY !!!
// This file was generated by the "generate_public_suffix_data" utility.
#include "PublicSuffixData.hpp"

#include "../../../../mem/ByteBlockLiteral.hpp"
#include "../../../../text/Literals.hpp"

#include <array>
#include <cstdint>

namespace erbsland::network::impl::public_suffix_data {{

using namespace text::literals;

auto version() noexcept -> text::StringLiteral {{
    return "{version}"_el;
}}

auto data() noexcept -> PublicSuffixDataView {{
    // clang-format off
{arrays}
    // clang-format on
    static constexpr auto cEncodedStorageByteCount = std::size_t{{{data.encoded_storage_byte_count}U}};
    static constexpr auto cLegacy32StorageByteCount = std::size_t{{{legacy_32_size}U}};
    static_assert(cEncodedStorageByteCount * 4U <= cLegacy32StorageByteCount);
    // Name the constant-evaluated literals explicitly for MSVC aggregate initialization.
    static constexpr auto cEncodedRulesLiteral = mem::ByteBlockLiteral{{std::span<const std::uint8_t>{{cEncodedRules}}}};
    static constexpr auto cPrefixTreeLiteral = mem::ByteBlockLiteral{{std::span<const std::uint8_t>{{cPrefixTree}}}};
    static constexpr auto cContentTreeLiteral = mem::ByteBlockLiteral{{std::span<const std::uint8_t>{{cContentTree}}}};
    return PublicSuffixDataView{{
        .encodedRules = cEncodedRulesLiteral,
        .blockBitOffsets = cBlockBitOffsets,
        .prefixTree = cPrefixTreeLiteral,
        .contentTree = cContentTreeLiteral,
        .ruleCount = {len(data.rules)}U,
        .blockSize = {self.block_size}U,
        .maximumRuleLength = {data.maximum_rule_length}U,
        .maximumRuleLabelCount = {data.maximum_rule_label_count}U,
        .originalRuleTextByteCount = {data.original_rule_text_byte_count}U,
        .encodedStorageByteCount = cEncodedStorageByteCount,
    }};
}}

// Exact rules: {exact_count}; wildcard rules: {wildcard_count}; exception rules: {exception_count}.
// Encoded persistent data: {data.encoded_storage_byte_count} bytes.

}}
"""

    def run(self, argv=None) -> None:
        self.parse_command_line(argv)
        version, exact, wildcard, exception = self.parse()
        data = self.encode(exact, wildcard, exception)
        output = self.render(version, data)
        current = self.output_path.read_text(encoding="utf-8") if self.output_path.exists() else None
        if current == output:
            print("Unchanged generated Public Suffix List data.")
            return
        if self.check:
            raise RuntimeError("Generated Public Suffix List data is stale.")
        self.output_path.write_text(output, encoding="utf-8")
        print(f"Updated {self.output_path.relative_to(self.project_directory)}")
