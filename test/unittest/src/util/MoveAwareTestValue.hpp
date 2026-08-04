// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MoveAwareTestCounts.hpp"

#include <compare>
#include <memory>

namespace erbsland::test {

/// Stores an integer while recording copy and move operations for tests.
/// @notest{Used by tests to assert copy and move behavior.}
class MoveAwareTestValue final {
public:
    /// Shared pointer to the value's copy and move counters.
    using CountsPtr = std::shared_ptr<MoveAwareTestCounts>;

public:
    /// Create a default value without operation counters.
    MoveAwareTestValue() = default;
    /// Create a value with independent operation counters.
    explicit MoveAwareTestValue(const int value) : _value{value}, _counts{std::make_shared<MoveAwareTestCounts>()} {}
    /// Copy a value and increment its shared copy counter.
    MoveAwareTestValue(const MoveAwareTestValue &other) : _value{other._value}, _counts{other._counts} {
        if (_counts) {
            _counts->copies += 1;
        }
    }
    /// Move a value and increment its shared move counter.
    MoveAwareTestValue(MoveAwareTestValue &&other) noexcept : _value{other._value}, _counts{std::move(other._counts)} {
        if (_counts) {
            _counts->moves += 1;
        }
    }
    // defaults
    ~MoveAwareTestValue() = default;
    /// Copy a value and increment its shared copy counter.
    auto operator=(const MoveAwareTestValue &other) -> MoveAwareTestValue & {
        if (this != &other) {
            _value = other._value;
            _counts = other._counts;
            if (_counts) {
                _counts->copies += 1;
            }
        }
        return *this;
    }
    /// Move a value and increment its shared move counter.
    auto operator=(MoveAwareTestValue &&other) noexcept -> MoveAwareTestValue & {
        if (this != &other) {
            _value = other._value;
            _counts = std::move(other._counts);
            if (_counts) {
                _counts->moves += 1;
            }
        }
        return *this;
    }

public:
    /// Get the stored integer value.
    [[nodiscard]] auto value() const noexcept -> int { return _value; }
    /// Get the shared copy and move counters.
    [[nodiscard]] auto counts() const noexcept -> CountsPtr { return _counts; }

public: // operators
    /// Compare test values by their stored integer values.
    [[nodiscard]] auto operator<=>(const MoveAwareTestValue &other) const noexcept -> std::strong_ordering {
        return _value <=> other._value;
    }
    /// Test whether two test values have equal stored integer values.
    [[nodiscard]] auto operator==(const MoveAwareTestValue &other) const noexcept -> bool {
        return _value == other._value;
    }

private:
    int _value{0};
    CountsPtr _counts{};
};

}
