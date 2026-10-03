// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ProcessId.hpp"

#include <cstdint>

namespace erbsland::system::impl {

/// Internal access to the native representation of a process identifier.
/// @tested{ProcessInfoTest SubprocessInteropTest}
class ProcessIdAccess final {
public:
    /// Bind access to a process identifier that outlives this helper.
    /// @param value The process identifier.
    explicit constexpr ProcessIdAccess(const ProcessId &value) noexcept : _value{value} {}

public: // conversion
    /// Create a process identifier from a native numeric value.
    /// @param value The native identifier; zero represents an invalid identifier.
    /// @return The opaque process identifier.
    [[nodiscard]] static constexpr auto fromNative(const std::uint64_t value) noexcept -> ProcessId {
        return ProcessId{value};
    }
    /// Get the native numeric value of a process identifier.
    /// @return The native identifier, or zero for an invalid identifier.
    [[nodiscard]] constexpr auto toNative() const noexcept -> std::uint64_t { return _value._value; }

private:
    const ProcessId &_value; ///< The accessed identifier.
};

}
