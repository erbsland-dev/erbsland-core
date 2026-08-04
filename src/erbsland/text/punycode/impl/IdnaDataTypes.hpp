// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::punycode::impl {

/// The IDNA2008 derived status encoded in the generated attributes.
enum class IdnaStatus : uint8_t {
    Disallowed, ///< The code point is disallowed or unassigned.
    PValid,     ///< The code point is protocol-valid.
    ContextJ,   ///< The code point requires a CONTEXTJ rule.
    ContextO,   ///< The code point requires a CONTEXTO rule.
};

/// The bidi class required by RFC 5893.
enum class IdnaBidi : uint8_t {
    Unknown, ///< No bidi class is stored for this code point.
    L,       ///< Left-to-right.
    R,       ///< Right-to-left.
    AL,      ///< Arabic letter.
    EN,      ///< European number.
    AN,      ///< Arabic number.
    ES,      ///< European separator.
    CS,      ///< Common separator.
    ET,      ///< European terminator.
    ON,      ///< Other neutral.
    BN,      ///< Boundary neutral.
    NSM,     ///< Nonspacing mark.
};

/// The joining type required by RFC 5892 CONTEXTJ processing.
enum class IdnaJoining : uint8_t {
    Other, ///< No joining behavior relevant to IDNA.
    L,     ///< Left-joining.
    R,     ///< Right-joining.
    D,     ///< Dual-joining.
    T,     ///< Transparent.
};

/// A script required by RFC 5892 CONTEXTO processing.
enum class IdnaScript : uint8_t {
    None,     ///< No contextual script is stored.
    Greek,    ///< Greek.
    Han,      ///< Han.
    Hebrew,   ///< Hebrew.
    Hiragana, ///< Hiragana.
    Katakana, ///< Katakana.
};

/// The combined generated attributes for one Unicode code point.
/// @tested{IdnaTest}
class IdnaAttributes final {
public:
    /// Create empty, disallowed attributes.
    constexpr IdnaAttributes() noexcept = default;
    /// Create attributes from their generated representation.
    explicit constexpr IdnaAttributes(const uint16_t value) noexcept : _value{value} {}

public:
    /// The derived IDNA2008 status.
    [[nodiscard]] constexpr auto status() const noexcept -> IdnaStatus {
        return static_cast<IdnaStatus>(_value & 0x0003U);
    }
    /// The RFC 5893 bidi class.
    [[nodiscard]] constexpr auto bidi() const noexcept -> IdnaBidi {
        return static_cast<IdnaBidi>((_value >> 2U) & 0x000FU);
    }
    /// The RFC 5892 joining type.
    [[nodiscard]] constexpr auto joining() const noexcept -> IdnaJoining {
        return static_cast<IdnaJoining>((_value >> 6U) & 0x0007U);
    }
    /// Test if the canonical combining class is Virama.
    [[nodiscard]] constexpr auto isVirama() const noexcept -> bool { return (_value & 0x0200U) != 0U; }
    /// The script used by contextual rules.
    [[nodiscard]] constexpr auto script() const noexcept -> IdnaScript {
        return static_cast<IdnaScript>((_value >> 10U) & 0x0007U);
    }

private:
    uint16_t _value{}; ///< The compact 13-bit representation.
};

/// One generated inclusive range within a 16-bit Unicode page.
/// @tested{IdnaTest}
struct IdnaRange final {
    uint16_t first;            ///< The first page-local code point.
    uint16_t last;             ///< The last page-local code point.
    IdnaAttributes attributes; ///< The combined attributes for the range.
};

static_assert(sizeof(IdnaRange) == 6U);

}
