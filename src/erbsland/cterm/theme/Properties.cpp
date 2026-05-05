// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Properties.hpp"

namespace erbsland::cterm::theme {

void Properties::setBlock(const BlockRole role, const text::Char codePoint) noexcept {
    if (!_blocks.has_value()) {
        _blocks = Blocks{};
        _blocks->fill(cInheritedBlock);
    }
    (*_blocks)[static_cast<std::size_t>(role)] = codePoint;
}

}
