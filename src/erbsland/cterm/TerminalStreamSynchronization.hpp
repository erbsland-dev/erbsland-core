// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TerminalStream_fwd.hpp"

#include "impl/TerminalStreamData_fwd.hpp"

#include "../core/Definitions.hpp"

#include <memory>
#include <mutex>

namespace erbsland::cterm {

/// Shared synchronization state for terminal text streams.
/// @tested{TerminalStreamTest}
class TerminalStreamSynchronization final {
public:
    /// Create synchronization state for one terminal stream.
    TerminalStreamSynchronization() = default;

    // defaults
    ~TerminalStreamSynchronization() = default;
    TerminalStreamSynchronization(const TerminalStreamSynchronization &) = delete;
    auto operator=(const TerminalStreamSynchronization &) -> TerminalStreamSynchronization & = delete;
    TerminalStreamSynchronization(TerminalStreamSynchronization &&) = delete;
    auto operator=(TerminalStreamSynchronization &&) -> TerminalStreamSynchronization & = delete;

private:
    friend class TerminalStream;
    friend class impl::TerminalStreamData;

private:
    std::mutex _mutex; ///< Synchronizes writes to a shared terminal.
};

using TerminalStreamSynchronizationPtr = std::shared_ptr<TerminalStreamSynchronization>;

}
