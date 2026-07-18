// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AnyString.hpp"

#include "AnyStringEditor.hpp"

namespace erbsland::text {

AnyString::AnyString(const AnyStringEditor &str) noexcept : AnyString(str.toAnyString()) {
}

}
