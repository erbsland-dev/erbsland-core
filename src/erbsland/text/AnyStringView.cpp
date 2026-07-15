// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AnyStringView.hpp"

#include "AnyString.hpp"

namespace erbsland::text {

AnyStringView::AnyStringView(const AnyString &str) noexcept : AnyStringView(str.toAnyStringView()) {
}

}
