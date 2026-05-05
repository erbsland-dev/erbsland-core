// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::event {

class EventData;
using EventDataUniquePtr = std::unique_ptr<EventData>;

/// The base class for event data.
/// If an event has additional data, it is stored as a subclass of this class.
class EventData {
public:
    virtual ~EventData() = default;

public:
    /// Test if this event was handled and should not be propagated further.
    [[nodiscard]] virtual auto isHandled() const noexcept -> bool { return _handled; }
    /// Mark this event as handled and should not be propagated further.
    void setHandled() noexcept { _handled = true; }

private:
    bool _handled{false};
};

}
