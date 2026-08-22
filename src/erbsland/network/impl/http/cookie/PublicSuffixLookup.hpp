// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PublicSuffixData.hpp"

#include "../../../../mem/BitReader.hpp"
#include "../../../../text/String.hpp"

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::network::impl {

/// Decode and search the compressed Public Suffix List.
/// @tested{PublicSuffixListTest}
class PublicSuffixLookup final {
private:
    static constexpr auto cRuleBufferSize = std::size_t{256U};
    static constexpr auto cLeafMask = std::uint8_t{0x80U};

    /// The rule family encoded by a terminal token.
    enum class RuleType : std::uint8_t {
        Exact = 0U,     ///< An exact Public Suffix List rule.
        Wildcard = 1U,  ///< A wildcard Public Suffix List rule base.
        Exception = 2U, ///< An exception to a wildcard rule.
        None = 0xffU,   ///< No matching rule.
    };

    /// A decoded rule reused while scanning one prefix-compressed block.
    struct RuleBuffer final {
        std::array<char, cRuleBufferSize> characters{}; ///< Reversed-label ASCII rule characters.
        std::size_t length = 0U;                        ///< Active character count.
        RuleType type = RuleType::None;                 ///< Rule family encoded by the terminal token.
    };

public:
    /// Create a lookup over the generated read-only data.
    PublicSuffixLookup() noexcept;

    // defaults
    ~PublicSuffixLookup() = default;
    PublicSuffixLookup(const PublicSuffixLookup &) = default;
    PublicSuffixLookup(PublicSuffixLookup &&) = default;
    auto operator=(const PublicSuffixLookup &) -> PublicSuffixLookup & = default;
    auto operator=(PublicSuffixLookup &&) -> PublicSuffixLookup & = default;

public:
    /// Return the number of labels selected by the prevailing Public Suffix List rule.
    [[nodiscard]] auto publicSuffixLabelCount(const text::String &host) const -> std::size_t;

private:
    /// Decode a common-prefix-length token.
    [[nodiscard]] auto decodePrefixToken(mem::BitReader &reader) const -> std::size_t;
    /// Decode a rule-content token.
    [[nodiscard]] auto decodeContentToken(mem::BitReader &reader) const -> std::uint8_t;
    /// Decode the next rule into the reusable buffer.
    [[nodiscard]] auto decodeRule(mem::BitReader &reader, RuleBuffer &rule) const -> bool;
    /// Compare a decoded rule with a reversed-label lookup key.
    [[nodiscard]] static auto compare(const RuleBuffer &rule, std::span<const char> key) noexcept
        -> std::strong_ordering;
    /// Find the rule family for an exact reversed-label key.
    [[nodiscard]] auto findRule(std::span<const char> key) const -> RuleType;
    /// Decode an ASCII content token.
    [[nodiscard]] static auto characterFromToken(std::uint8_t token) noexcept -> char;

private:
    public_suffix_data::PublicSuffixDataView _data; ///< Generated immutable lookup tables and bounds.
};

}
