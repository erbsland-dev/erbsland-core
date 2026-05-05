// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "State.hpp"

#include <bit>
#include <cstdint>
#include <functional>
#include <initializer_list>

namespace erbsland::cterm::theme {

/// A compact set of UI state flags used in theme selector matching.
class States final {
public:
    /// Create an empty state set.
    constexpr States() noexcept = default;
    /// Create a state set containing one state.
    /// @param state The initial state.
    constexpr States(const State state) noexcept : _bits{state.mask()} {}
    /// Create a state set from raw bits.
    /// @param bits The raw state bits.
    explicit constexpr States(const uint8_t bits) noexcept : _bits{bits} {}
    /// Create a state set from an initializer list.
    /// @param states The states to add.
    States(std::initializer_list<State> states) noexcept;

public: // operators
    /// Compare two state sets.
    auto operator==(const States &other) const noexcept -> bool = default;
    /// Compare two state sets.
    auto operator!=(const States &other) const noexcept -> bool = default;
    /// Combine two state sets.
    [[nodiscard]] auto operator|(const States &other) const noexcept -> States {
        return States{static_cast<uint8_t>(_bits | other._bits)};
    }
    /// Add all states from another state set.
    auto operator|=(const States &other) noexcept -> States & {
        _bits |= other._bits;
        return *this;
    }

public:
    /// Add one state.
    /// @param state The state to add.
    void add(const State state) noexcept { _bits = static_cast<uint8_t>(_bits | state.mask()); }
    /// Remove one state.
    /// @param state The state to remove.
    void remove(const State state) noexcept { _bits = static_cast<uint8_t>(_bits & ~state.mask()); }
    /// Test if one state is present.
    /// @param state The state to test.
    [[nodiscard]] constexpr auto contains(const State state) const noexcept -> bool {
        return (state.mask() & _bits) == state.mask();
    }
    /// Test if all states from another set are present.
    /// @param states The required states.
    [[nodiscard]] constexpr auto containsAll(const States states) const noexcept -> bool {
        return (states._bits & _bits) == states._bits;
    }
    /// Test if the set is empty.
    [[nodiscard]] constexpr auto empty() const noexcept -> bool { return _bits == 0; }
    /// Get the number of states in the set.
    [[nodiscard]] auto size() const noexcept -> int { return static_cast<int>(std::popcount(_bits)); }
    /// Get the raw state bits.
    [[nodiscard]] constexpr auto bits() const noexcept -> uint8_t { return _bits; }
    /// Get a stable hash for this state set.
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t { return static_cast<std::size_t>(_bits); }

private:
    uint8_t _bits{0}; ///< The raw state bits.
};

}

template <>
struct std::hash<erbsland::cterm::theme::States> {
    auto operator()(const erbsland::cterm::theme::States states) const noexcept -> std::size_t { return states.hash(); }
};
