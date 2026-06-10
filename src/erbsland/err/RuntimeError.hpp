// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Exception.hpp"

namespace erbsland::err {

/// A runtime exception.
/// Runtime exceptions are meany to be caught and handled in user-code.
class RuntimeError : public Exception {
public:
    using Exception::Exception;
    ~RuntimeError() override = default;
};

}
