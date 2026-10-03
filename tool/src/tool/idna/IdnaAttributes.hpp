// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IdnaBidi.hpp"
#include "IdnaJoining.hpp"
#include "IdnaScript.hpp"
#include "IdnaStatus.hpp"

#include <cstdint>

namespace tool::idna {

/// Access source-derived validation attributes without a compiled Unicode table.
/// @tested{UnicodeSourceTest PublicSuffixFullTest}
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

}
