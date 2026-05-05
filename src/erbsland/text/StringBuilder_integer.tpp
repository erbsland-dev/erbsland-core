// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/IntegerAppend_fwd.hpp"

namespace erbsland::text {

template <math::AnyIntegerType T>
auto StringBuilder::appendInteger(T value, IntegerFormat format) -> StringBuilder & {
    impl::appendInteger(*this, value, format);
    return *this;
}

}

#include "impl/IntegerAppend.hpp"
