// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockPrintContextToCursorWriter.hpp"

namespace erbsland::cterm::impl {

BlockPrintContextToCursorWriter::BlockPrintContextToCursorWriter(CursorWriter &writer) noexcept :
    BlockPrintContextToString{writer.style(), true}, _writer{writer} {
}

void BlockPrintContextToCursorWriter::commit() noexcept {
    if (!_builder.isEmpty()) {
        _writer.writeResolved(_builder.takeString());
    }
    _writer.setStyle(_style);
}

}
