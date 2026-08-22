// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationPartThread.hpp"

#include "../ApplicationPart_fwd.hpp"
#include "../ApplicationPartIdentifier_fwd.hpp"
#include "../ApplicationPartState.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace erbsland::core::impl {

/// Runtime and graph data for one prepared application part.
/// @tested{ApplicationPartManagerTest}
struct ApplicationPartRecord {
    ApplicationPartIdentifierPtr identifier;                         ///< Stable public identifier.
    std::vector<std::size_t> dependencies;                           ///< Manager-local dependency numbers.
    std::vector<std::size_t> dependents;                             ///< Manager-local dependent numbers.
    ApplicationPartPtr part;                                         ///< Prepared part instance.
    ApplicationPartThreadPtr thread;                                 ///< Dedicated runtime thread.
    ApplicationPartState state{ApplicationPartState::Uninitialized}; ///< Current lifecycle state.
    bool automaticEvaluated{false};                                  ///< True after automatic-start evaluation.
    bool manualRequested{false};                                     ///< True if manual startup requested this part.
    bool stopRequested{false};                                       ///< True if stopping was requested.
    bool failed{false};                                              ///< Sticky failure flag for the terminal state.
    uint64_t timeoutGeneration{0};                                   ///< Invalidates stale timeout callbacks.
};

}
