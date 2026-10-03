// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Exception.hpp"

namespace erbsland::err {

/// A fatal programming, invariant, or system-resource failure.
/// This class of exceptions is not meant to be caught and handled in user-code, instead they shall cause the
/// program to abort.
/// @tested{ExceptionHierarchyTest}
class LogicFailure : public Exception {
public:
    using Exception::Exception;
    // defaults
    ~LogicFailure() override = default;
};

}
