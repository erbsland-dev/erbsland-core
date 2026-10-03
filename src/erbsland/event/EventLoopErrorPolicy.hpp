// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::event {

/// The default policy for event callback `err::RuntimeError` failures.
/// Other exceptions bypass the policy and propagate to the loop caller.
enum class EventLoopErrorPolicy : uint8_t {
    Ignore,          ///< Discard the exception and continue.
    Store,           ///< Store the exception and continue.
    Quit,            ///< Store the exception and gracefully quit the loop.
    FailApplication, ///< Store and forward the exception to the owning application.
};

}
