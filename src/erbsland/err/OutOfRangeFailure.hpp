// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogicFailure.hpp"

namespace erbsland::err {

/// A bounds or capacity violation of an API precondition.
/// @tested{ExceptionHierarchyTest}
class OutOfRangeFailure : public LogicFailure {
public:
    /// Create a failure with a reason.
    /// @param reason The reason for the failure.
    explicit OutOfRangeFailure(text::String reason) noexcept : LogicFailure{std::move(reason)} {}
    /// @overload
    explicit OutOfRangeFailure(const std::string_view reason) noexcept : LogicFailure{reason} {}

    // defaults
    ~OutOfRangeFailure() override = default;
};

}
