// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace erbsland::event {

class Scheduler;

/// Represents a scheduled action.
class ScheduledActionRef {
    friend class Scheduler;
    /// A special ID that indicates that no action is scheduled.
    constexpr static auto cNoScheduledActionId = uint64_t{0};

public:
    /// Creates a "no scheduled action" reference.
    ScheduledActionRef() = default;
    ~ScheduledActionRef() = default;
    ScheduledActionRef(const ScheduledActionRef &) = default;
    auto operator=(const ScheduledActionRef &) -> ScheduledActionRef & = default;

private:
    /// Creates a scheduled action reference with the given identifier.
    constexpr explicit ScheduledActionRef(const uint64_t identifier) : _identifier{identifier} {};

public:
    auto operator==(const ScheduledActionRef &other) const -> bool { return _identifier == other._identifier; }
    auto operator!=(const ScheduledActionRef &other) const -> bool { return !(*this == other); }

public:
    /// Test if this scheduled action reference is valid.
    [[nodiscard]] auto isValid() const -> bool { return _identifier != cNoScheduledActionId; }
    /// Get a hash for this reference.
    [[nodiscard]] auto hash() const noexcept -> std::size_t { return _identifier; }

private:
    uint64_t _identifier = cNoScheduledActionId;
};

}

template <>
struct std::hash<erbsland::event::ScheduledActionRef> {
    auto operator()(const erbsland::event::ScheduledActionRef &character) const noexcept -> std::size_t {
        return character.hash();
    }
};
