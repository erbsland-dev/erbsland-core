// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Subprocess.hpp"

#include "../event/EventBackendId.hpp"
#include "../event/EventRegistry.hpp"

namespace erbsland::system {

/// Event-loop frontend for system event sources.
/// @tested{SubprocessEventTest}
class System {
    friend class Subprocess;

public:
    // defaults
    virtual ~System() = default;

public: // factories
    /// Get the system backend identifier.
    /// @return The built-in system backend identifier.
    [[nodiscard]] static constexpr auto backendId() noexcept -> event::EventBackendId {
        return event::id::SystemBackend;
    }
    /// Prepare an owned child bound to this loop; call on its owner loop.
    /// @param executable Direct executable path.
    /// @param arguments Child arguments.
    /// @param options Launch and output configuration.
    /// @return A prepared event source; start it after configuring handlers.
    [[nodiscard]] virtual auto createSubprocess(
        const path::Path &executable, const text::StringList &arguments = {}, const SubprocessOptions &options = {})
        -> SubprocessPtr = 0;

private:
    /// Register a manually created process on its lazily selected owner loop.
    virtual void bindSubprocess(const SubprocessPtr &process) = 0;
};

}
