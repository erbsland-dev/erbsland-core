// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProcessSignal.hpp"

#include <functional>
#include <memory>

namespace erbsland::system::impl {

/// Native backend for forwarding process signals to a regular thread.
/// @notest{Abstract platform interface; concrete backends own the tests.}
class ProcessSignalBackend {
public:
    using SignalFn = std::function<void(ProcessSignal)>;

public:
    // defaults
    virtual ~ProcessSignalBackend() = default;

public:
    /// Terminate using the native default behavior after an unclaimed signal.
    virtual void terminateWithDefault(ProcessSignal signal) noexcept = 0;

public:
    /// Create the platform backend.
    [[nodiscard]] static auto create(SignalFn signalFn) -> std::unique_ptr<ProcessSignalBackend>;
};

}
