// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LayoutResult.hpp"

#include <utility>

namespace erbsland::cterm::impl::paragraph {

void LayoutResult::appendEmptyLine() {
    _lines.emplace_back();
}

void LayoutResult::appendLine(LayoutLine line) {
    _lines.emplace_back(std::move(line));
}

}
