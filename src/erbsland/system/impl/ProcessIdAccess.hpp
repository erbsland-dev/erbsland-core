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
    /// Create a process identifier from a native numeric value.
    [[nodiscard]] static constexpr auto fromNative(const std::uint64_t value) noexcept -> ProcessId {
        return value == 0U ? ProcessId{} : ProcessId{value};
    }
    /// Get the native numeric value of a process identifier.
    [[nodiscard]] static constexpr auto toNative(const ProcessId value) noexcept -> std::uint64_t {
        return value._value;
    }
};

}
