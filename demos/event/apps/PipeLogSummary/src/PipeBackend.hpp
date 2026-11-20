// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PosixPipeDriver.hpp"

#include <erbsland/EventBackend.hpp>

#include <functional>

namespace demo {

/// Queues one input chunk for ordinary dispatch after native readiness.
class PipeBackend final : public el::EventBackend {
public:
    /// Connect this input source to the application's log consumer.
    /// @param registry Application registry that allocates stable routing identifiers.
    /// @param consume Receives bytes during dispatch, including an empty block at EOF.
    PipeBackend(el::EventRegistry &registry, std::function<void(const el::ByteBlock &)> consume);

public: // implement EventBackend
    auto backendId() const noexcept -> el::EventBackendId override;
    void attach(el::EventBackendTargetWeakPtr target, el::EventLoopDriverWeakPtr driver) override;
    void poll(el::TimePoint now) override;
    auto handleEvent(const el::Event &event) -> bool override;
    auto nextWakeTime() const -> std::optional<el::TimePoint> override;

private:
    el::EventBackendId _id;                              ///< Registered backend family.
    el::EventId _inputEvent;                             ///< Delivers the pending chunk.
    std::function<void(const el::ByteBlock &)> _consume; ///< Owner-loop consumer.
    std::shared_ptr<PosixPipeDriver> _driver;            ///< Typed access to this application's input.
    el::EventBackendTargetWeakPtr _target;               ///< Loop admission without a reference cycle.
    std::optional<el::ByteBlock> _pending;               ///< Owned until the queued event is dispatched.
};

}
