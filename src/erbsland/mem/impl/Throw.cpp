// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Throw.hpp"

#include "../../err/OutOfRangeError.hpp"

namespace erbsland::mem::impl {

void throwOutOfRange(const std::string_view reason) {
    throw err::OutOfRangeError{reason};
}

}
