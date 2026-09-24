// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteBlockLiteral.hpp"
#include "../../../../text/StringLiteral.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::network::impl::public_suffix_data {

/// A read-only view of the generated compressed Public Suffix List.
/// @tested{PublicSuffixListTest}
struct PublicSuffixDataView final {
    mem::ByteBlockLiteral encodedRules;             ///< Huffman-coded reversed-label rule records.
    std::span<const std::uint32_t> blockBitOffsets; ///< Bit offsets for independently decodable rule blocks.
    mem::ByteBlockLiteral prefixTree;               ///< Flattened Huffman tree for prefix lengths.
    mem::ByteBlockLiteral contentTree;              ///< Flattened Huffman tree for rule content.
    std::size_t ruleCount;                          ///< Number of encoded exact, wildcard, and exception rules.
    std::size_t blockSize;                          ///< Maximum number of records decoded for one block scan.
    std::size_t maximumRuleLength;                  ///< Maximum byte length of a reversed-label rule.
    std::size_t maximumRuleLabelCount;              ///< Maximum number of labels in a stored rule base.
    std::size_t originalRuleTextByteCount;          ///< Original rule text bytes including terminators.
    std::size_t encodedStorageByteCount;            ///< Persistent byte count of all encoded lookup tables.
};

/// Get the pinned Public Suffix List version.
/// @tested{PublicSuffixListTest}
[[nodiscard]] auto version() noexcept -> text::StringLiteral;
/// Get the generated compressed Public Suffix List data.
/// @tested{PublicSuffixListTest}
[[nodiscard]] auto data() noexcept -> PublicSuffixDataView;

}
