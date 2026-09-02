// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>

namespace erbsland::system {

/// A supported operating-system family.
/// @tested{SystemInfoTest}
class OperatingSystem final {
public:
    /// Operating-system family values.
    enum Value : std::uint8_t {
        Unknown, ///< An unknown or unsupported operating system.
        Windows, ///< Microsoft Windows.
        Macos,   ///< Apple macOS.
        Linux,   ///< Linux.
    };

public:
    /// Create an operating-system value from its raw value.
    constexpr OperatingSystem(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    constexpr OperatingSystem() noexcept = default;
    ~OperatingSystem() = default;
    OperatingSystem(const OperatingSystem &) = default;
    OperatingSystem(OperatingSystem &&) = default;
    auto operator=(const OperatingSystem &) -> OperatingSystem & = default;
    auto operator=(OperatingSystem &&) -> OperatingSystem & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const OperatingSystem &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const OperatingSystem &other, value, other._value);

public: // accessors
    /// Get the raw operating-system value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Get the stable lowercase operating-system identifier.
    [[nodiscard]] auto toString() const -> text::String;

private:
    Value _value{Unknown}; ///< Operating-system family value.
};

}
