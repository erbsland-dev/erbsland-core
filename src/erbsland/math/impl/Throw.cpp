// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Throw.hpp"

#include "../../err/OverflowError.hpp"

namespace erbsland::math::impl {

void throwOverflow(const std::string_view reason) {
    throw err::OverflowError{reason};
}

}
