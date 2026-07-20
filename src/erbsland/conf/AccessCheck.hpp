// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AccessCheckResult.hpp"
#include "AccessSources.hpp"

#include <memory>

namespace erbsland::conf {

class AccessCheck;
using AccessCheckPtr = std::shared_ptr<AccessCheck>;

/// The interface to access check implementations.
/// @tested{ParserAccessTest}
class AccessCheck {
public:
    /// Default destructor.
    virtual ~AccessCheck() = default;

public:
    /// The check function is called for every source, including the initial source that is passed to the
    /// `parse()` function call. You can either grant or deny access to this source. If you deny the access to
    /// the source, the parser will throw a `ConfError` with `ConfErrorCategory::Access`.
    /// Instead of returning `AccessResult::Denied`, you can also throw a `ConfError` with `ConfErrorCategory::Access`.
    /// @param sources The sources that are verified.
    /// @return Return either `AccessResult::Granted` or `AccessResult::Denied`.
    /// @throws ConfError Alternatively, throw a `ConfError` with the `ConfErrorCategory::Access`.
    virtual auto check(const AccessSources &sources) -> AccessCheckResult = 0;
};

}
