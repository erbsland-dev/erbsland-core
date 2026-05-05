// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventData.hpp"

namespace erbsland::event {

/// The data for a quit event.
class QuitEvent : public EventData {
public:
    /// Create a quit event with exit code `0`.
    QuitEvent() = default;
    /// Create a quit event with an explicit exit code.
    /// @param exitCode The requested application exit code.
    explicit QuitEvent(const int exitCode) : _exitCode(exitCode) {}

public:
    /// Get the requested application exit code.
    /// @return The exit code.
    [[nodiscard]] auto exitCode() const -> int { return _exitCode; }

private:
    int _exitCode{0};
};

}
