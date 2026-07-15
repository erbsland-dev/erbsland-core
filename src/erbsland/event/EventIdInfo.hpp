// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/StringView.hpp"

#include <utility>

namespace erbsland::event {

/// Information about an event id for diagnostics.
class EventIdInfo {
public:
    /// Create new event identifier information.
    EventIdInfo(text::StringView name, text::StringView description) :
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
    [[nodiscard]] auto name() const noexcept -> const text::StringView & { return _name; }
    /// Get the event description.
    [[nodiscard]] auto description() const noexcept -> const text::StringView & { return _description; }

private:
    text::StringView _name;        ///< The reverse DNS style name of the event.
    text::StringView _description; ///< A description of the event identifier.
};

}
