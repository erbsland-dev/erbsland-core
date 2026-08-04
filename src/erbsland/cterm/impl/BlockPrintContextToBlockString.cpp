// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockPrintContextToBlockString.hpp"

namespace erbsland::cterm::impl {

BlockPrintContextToBlockString::BlockPrintContextToBlockString(BlockStringEditor &text) noexcept : _text{text} {
}

void BlockPrintContextToBlockString::commit() noexcept {
    if (!_builder.isEmpty()) {
        _text.appendString(_builder.takeString(), {});
    }
}

}
