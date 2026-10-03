// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationData_fwd.hpp"
#include "ApplicationExecutionContext_fwd.hpp"

namespace erbsland::core::impl {

/// Retain the originating application services while synchronous background work runs on another thread.
/// Only internal data access is scoped; public application access and lifecycle management are unaffected.
/// @tested{TlsBackgroundOperationTest}
class ApplicationExecutionContext final {
public:
    /// Install the captured services on the current thread until this scope exits.
    /// @param data Nonnull originating application data retained before work submission.
    explicit ApplicationExecutionContext(ApplicationDataPtr data);
    /// Restore the preceding thread-local context before releasing captured services.
    ~ApplicationExecutionContext();

    // defaults/deletions
    ApplicationExecutionContext(const ApplicationExecutionContext &) = delete;
    auto operator=(const ApplicationExecutionContext &) -> ApplicationExecutionContext & = delete;
    ApplicationExecutionContext(ApplicationExecutionContext &&) = delete;
    auto operator=(ApplicationExecutionContext &&) -> ApplicationExecutionContext & = delete;

public: // context access
    /// Retain the installed thread-local data, or return null when ordinary instance management applies.
    [[nodiscard]] static auto current() noexcept -> ApplicationDataPtr;

private:
    ApplicationDataPtr _data; ///< Originating application services.
    /// Enclosing scope, which outlives this scope.
    /// @remark Review by TER: Using a raw-pointer is accepted for this use-case: See `_current`.
    ApplicationExecutionContext *_previous;
    /// Innermost synchronous data scope.
    /// @remark Review by TER: Using a raw-pointer is accepted for this use-case:
    ///     Requires same-thread, reverse-order destruction for correct context restoration.
    ///     Wrong order destruction is guarded and does terminate.
    static thread_local ApplicationExecutionContext *_current;
};

}
