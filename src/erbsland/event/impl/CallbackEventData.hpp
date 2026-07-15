// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../EventCallback.hpp"
#include "../EventData.hpp"

#include <utility>

namespace erbsland::event::impl {

/// Event data for invoked callbacks.
/// @tested{EventLoopTest}
class CallbackEventData final : public EventData {
public:
    /// Create callback event data.
    explicit CallbackEventData(EventCallback callback) : _callback{std::move(callback)} {}

public: // accessors
    /// Get the callback.
    [[nodiscard]] auto callback() const noexcept -> const EventCallback & { return _callback; }

private:
    EventCallback _callback; ///< The callback to execute.
};

}
