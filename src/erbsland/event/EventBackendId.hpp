// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace erbsland::event {

class EventRegistry;

/// Represents an event backend type.
/// @tested{EventRegistryTest EventBackendTest}
class EventBackendId final {
    friend class EventRegistry;

public:
    /// The underlying value type.
    using Value = uint32_t;

public:
    /// Create a "no backend" type.
    constexpr EventBackendId() noexcept = default;

    // defaults
    EventBackendId(const EventBackendId &) noexcept = default;
    EventBackendId(EventBackendId &&) noexcept = default;
    auto operator=(const EventBackendId &) noexcept -> EventBackendId & = default;
    auto operator=(EventBackendId &&) noexcept -> EventBackendId & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_rawValue, const EventBackendId &other, other._rawValue);

public: // tests/accessors
    /// Test if this backend type is valid.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _rawValue != 0; }
    /// Get the raw value of this backend type.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _rawValue; }

private:
    constexpr explicit EventBackendId(const Value rawValue) noexcept : _rawValue{rawValue} {}

private:
    Value _rawValue{0};
};

}

template <>
struct std::hash<erbsland::event::EventBackendId> {
    auto operator()(const erbsland::event::EventBackendId &value) const noexcept -> std::size_t {
        return std::hash<erbsland::event::EventBackendId::Value>{}(value.toRawValue());
    }
};
