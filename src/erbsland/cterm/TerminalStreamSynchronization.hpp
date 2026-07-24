// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <memory>
#include <mutex>

namespace erbsland::cterm {

class TerminalStream;

/// Shared synchronization state for terminal text streams.
/// @tested{TerminalStreamTest}
class TerminalStreamSynchronization final {
public:
    TerminalStreamSynchronization() = default;

    // defaults
    ~TerminalStreamSynchronization() = default;
    TerminalStreamSynchronization(const TerminalStreamSynchronization &) = delete;
    auto operator=(const TerminalStreamSynchronization &) -> TerminalStreamSynchronization & = delete;
    TerminalStreamSynchronization(TerminalStreamSynchronization &&) = delete;
    auto operator=(TerminalStreamSynchronization &&) -> TerminalStreamSynchronization & = delete;

private:
    friend class TerminalStream;

private:
    std::mutex _mutex; ///< Synchronizes writes to a shared terminal.
};

using TerminalStreamSynchronizationPtr = std::shared_ptr<TerminalStreamSynchronization>;

}
