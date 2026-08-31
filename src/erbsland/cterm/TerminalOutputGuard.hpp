// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TerminalOutputGuard_fwd.hpp"

#include <memory>
#include <mutex>

namespace erbsland::cterm {

/// An exclusive lease for a sequence of terminal output operations.
/// @tested{TerminalStreamTest}
class TerminalOutputGuard final {
    friend class Terminal;

private:
    /// Acquire the given terminal output mutex.
    /// @param mutex The terminal-owned recursive mutex to retain and lock.
    explicit TerminalOutputGuard(std::shared_ptr<std::recursive_mutex> mutex) :
        _mutex{std::move(mutex)}, _lock{*_mutex} {}

public:
    // defaults/deletions
    ~TerminalOutputGuard() = default;
    TerminalOutputGuard(const TerminalOutputGuard &) = delete;
    TerminalOutputGuard(TerminalOutputGuard &&) noexcept = default;
    auto operator=(const TerminalOutputGuard &) -> TerminalOutputGuard & = delete;
    auto operator=(TerminalOutputGuard &&) noexcept -> TerminalOutputGuard & = default;

private:
    std::shared_ptr<std::recursive_mutex> _mutex; ///< Keep synchronization alive for the guard lifetime.
    std::unique_lock<std::recursive_mutex> _lock; ///< The acquired terminal output lock.
};

}
