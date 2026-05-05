// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../EventData.hpp"
#include "../StopToken.hpp"

#include <functional>
#include <utility>

namespace erbsland::event::impl {

/// Internal event payload for invoked functions.
class InvocationEvent final : public EventData {
public:
    using InvocationFn = std::function<void(StopToken)>;

public:
    /// Create an invocation event for the given callback.
    /// @param invocation The callback to execute.
    explicit InvocationEvent(InvocationFn invocation) : _invocation{std::move(invocation)} {}

public:
    /// Execute the stored callback.
    /// @param stopToken The cooperative stop token for this invocation.
    void invoke(const StopToken stopToken) const {
        if (_invocation) {
            _invocation(stopToken);
        }
    }

private:
    InvocationFn _invocation; ///< The callback to execute.
};

}
