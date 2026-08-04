// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Exception.hpp"

namespace erbsland::err {

/// An error that indicates a faulty logic in the program.
/// This class of exceptions is not meant to be caught and handled in user-code, instead they shall cause the
/// program to abort.
class LogicError : public Exception {
public:
    using Exception::Exception;
    // defaults
    ~LogicError() override = default;
};

}
