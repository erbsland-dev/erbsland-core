// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../core/impl/WindowsApi.hpp"

namespace erbsland::event::impl {

/// Injectable native operations for the Windows event-loop driver.
/// @tested{WindowsEventLoopDriverTest}
class WindowsEventLoopNativeCalls {
public:
    virtual ~WindowsEventLoopNativeCalls() = default;

public: // native operations
    /// Post a wake completion; follows PostQueuedCompletionStatus return and last-error conventions.
    /// @param port The completion port.
    /// @param key The reserved wake completion key.
    /// @return Nonzero on success, or zero with the Windows last-error code set.
    virtual auto postWake(HANDLE port, ULONG_PTR key) noexcept -> BOOL;
};

}
