// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/StringView.hpp"

#include <utility>

namespace erbsland::event {

/// Information about a backend id for diagnostics.
/// @tested{EventRegistryTest}
class EventBackendIdInfo {
public:
    /// Create new backend identifier information.
    EventBackendIdInfo(text::StringView name, text::StringView description) :
        _name{std::move(name)}, _description{std::move(description)} {}

    // defaults
    EventBackendIdInfo() = default;
    ~EventBackendIdInfo() = default;
    EventBackendIdInfo(const EventBackendIdInfo &) = default;
    EventBackendIdInfo(EventBackendIdInfo &&) = default;
    auto operator=(const EventBackendIdInfo &) -> EventBackendIdInfo & = default;
    auto operator=(EventBackendIdInfo &&) -> EventBackendIdInfo & = default;

public:
    /// Get the reverse-DNS-style backend name.
    [[nodiscard]] auto name() const noexcept -> const text::StringView & { return _name; }
    /// Get the backend description.
    [[nodiscard]] auto description() const noexcept -> const text::StringView & { return _description; }

private:
    text::StringView _name;        ///< The reverse DNS style name of the backend.
    text::StringView _description; ///< A description of the backend identifier.
};

}
