// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StandardStreamSlot.hpp"

#include "../TextInputStream.hpp"
#include "../TextOutputStream.hpp"

#include <mutex>

namespace erbsland::stream::impl {

/// Stored state for an active standard stream redirection.
/// @tested{StandardStreamsTest}
class StandardStreamRedirectData final {
public:
    /// Create redirect data from previous stream targets.
    StandardStreamRedirectData(
        StandardStreamSlot slot,
        TextInputStreamPtr previousInput,
        TextOutputStreamPtr previousOutput,
        TextOutputStreamPtr previousError);

    // defaults
    ~StandardStreamRedirectData() = default;
    StandardStreamRedirectData(const StandardStreamRedirectData &) = delete;
    auto operator=(const StandardStreamRedirectData &) -> StandardStreamRedirectData & = delete;
    StandardStreamRedirectData(StandardStreamRedirectData &&) = delete;
    auto operator=(StandardStreamRedirectData &&) -> StandardStreamRedirectData & = delete;

public:
    /// Test if this redirection is still active.
    [[nodiscard]] auto isActive() const noexcept -> bool;
    /// Restore previous targets.
    void reset() noexcept;

private:
    StandardStreamSlot _slot;            ///< The replaced stream slot.
    TextInputStreamPtr _previousInput;   ///< The previous input target.
    TextOutputStreamPtr _previousOutput; ///< The previous output target.
    TextOutputStreamPtr _previousError;  ///< The previous error target.
    bool _active{true};                  ///< Whether this redirect is active.
    mutable std::mutex _mutex;           ///< Synchronizes reset operations.
};

}
