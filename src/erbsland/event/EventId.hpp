// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventIdRegistry_fwd.hpp"

#include "../util/impl/ComparisonHelper.hpp"

#include <cstddef>
#include <functional>

namespace erbsland::event {

/// Represents a type of event.
class EventId final {
    friend class EventIdRegistry;

public:
    /// The underlying value type.
    using Value = uint32_t;

public:
    /// Create a "no event" type.
    constexpr EventId() noexcept = default;

    // defaults
    EventId(const EventId &) noexcept = default;
    EventId(EventId &&) noexcept = default;
    auto operator=(const EventId &) noexcept -> EventId & = default;
    auto operator=(EventId &&) noexcept -> EventId & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_rawValue, const EventId &other, other._rawValue);

public: // tests/accessors
    /// Test if this event type is valid.
    /// All types, except "no event" are valid.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _rawValue != 0; }
    /// Get the raw value of this event type.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _rawValue; }

private:
    constexpr explicit EventId(const Value rawValue) noexcept : _rawValue{rawValue} {}

private:
    Value _rawValue{0};
};

}

template <>
struct std::hash<erbsland::event::EventId> {
    auto operator()(const erbsland::event::EventId &value) const noexcept -> std::size_t {
        return std::hash<erbsland::event::EventId::Value>{}(value.toRawValue());
    }
};
