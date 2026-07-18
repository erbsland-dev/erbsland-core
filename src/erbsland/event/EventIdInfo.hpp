// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"

#include <utility>

namespace erbsland::event {

/// Information about an event id for diagnostics.
class EventIdInfo {
public:
    /// Create new event identifier information.
    EventIdInfo(text::String name, text::String description) :
        _name{std::move(name)}, _description{std::move(description)} {}

    // defaults
    EventIdInfo() = default;
    ~EventIdInfo() = default;
    EventIdInfo(const EventIdInfo &) = default;
    EventIdInfo(EventIdInfo &&) = default;
    auto operator=(const EventIdInfo &) -> EventIdInfo & = default;
    auto operator=(EventIdInfo &&) -> EventIdInfo & = default;

public:
    /// Get the reverse-DNS-style event name.
    [[nodiscard]] auto name() const noexcept -> const text::String & { return _name; }
    /// Get the event description.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _description; }

private:
    text::String _name;        ///< The reverse DNS style name of the event.
    text::String _description; ///< A description of the event identifier.
};

}
