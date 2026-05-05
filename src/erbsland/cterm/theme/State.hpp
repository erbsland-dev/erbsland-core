// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm::theme {

/// A single UI state flag used in theme selectors.
class State final {
public:
    /// Create an empty state flag.
    constexpr State() noexcept = default;
    /// Create a state from a bit mask.
    /// @param mask The state bit mask.
    explicit constexpr State(const uint8_t mask) noexcept : _mask{mask} {}

public: // operators
    /// Compare two state flags.
    auto operator==(const State &other) const noexcept -> bool = default;
    /// Compare two state flags.
    auto operator!=(const State &other) const noexcept -> bool = default;

public:
    /// Get the bit mask for this state.
    /// @return The bit mask.
    [[nodiscard]] constexpr auto mask() const noexcept -> uint8_t { return _mask; }
    /// Test if this state has a non-zero bit mask.
    /// @return `true` if this state is usable in a selector.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _mask != 0; }

public:
    /// The surface has keyboard focus.
    static const State Focused;
    /// The surface contains keyboard focus in its subtree.
    static const State FocusWithin;
    /// The surface or item is selected.
    static const State Selected;
    /// The surface or item is disabled.
    static const State Disabled;
    /// The surface or item is checked.
    static const State Checked;

private:
    uint8_t _mask{0}; ///< The state bit mask.
};

inline constexpr State State::Focused{1U << 0U};
inline constexpr State State::FocusWithin{1U << 1U};
inline constexpr State State::Selected{1U << 2U};
inline constexpr State State::Disabled{1U << 3U};
inline constexpr State State::Checked{1U << 4U};

}
