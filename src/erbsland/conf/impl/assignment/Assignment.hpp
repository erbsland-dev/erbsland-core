// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AssignmentType.hpp"

#include "../value/Value.hpp"

#include "../../Location.hpp"
#include "../../NamePath.hpp"

#include <utility>

namespace erbsland::conf::impl {

/// A value assignment from the assignment stream.
class Assignment final {
public:
    /// Create a new assignment.
    template <typename NamePathFwd, typename LocationFwd>
        requires(std::convertible_to<NamePath, NamePathFwd> && std::convertible_to<Location, LocationFwd>)
    Assignment(const AssignmentType type, NamePathFwd &&namePath, LocationFwd &&nameLocation, ValuePtr value) :
        _type{type},
        _namePath{std::forward<NamePathFwd>(namePath)},
        _nameLocation{std::forward<LocationFwd>(nameLocation)},
        _value{std::move(value)} {}

    // defaults
    Assignment() = default; // end-of-document.
    Assignment(const Assignment &) = default;
    Assignment(Assignment &&) = default;
    auto operator=(const Assignment &) -> Assignment & = default;
    auto operator=(Assignment &&) -> Assignment & = default;
    ~Assignment() = default;

public: // accessors
    /// Access the type of this assignment.
    [[nodiscard]] auto type() const noexcept -> AssignmentType { return _type; }

    /// Access the name of this assignment.
    [[nodiscard]] auto namePath() const noexcept -> const NamePath & { return _namePath; }

    /// Access the location of this assignment.
    [[nodiscard]] auto location() const noexcept -> const Location & { return _nameLocation; }

    /// Access the value of this assignment.
    [[nodiscard]] auto value() const noexcept -> const ValuePtr & { return _value; }

private:
    AssignmentType _type{AssignmentType::EndOfDocument};
    NamePath _namePath;
    Location _nameLocation;
    ValuePtr _value;
};

}
