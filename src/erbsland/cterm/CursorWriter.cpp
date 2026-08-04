// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CursorWriter.hpp"

#include "impl/BlockPrintContextToCursorWriter.hpp"
#include "impl/BlockPrintContextToString.hpp"

namespace erbsland::cterm {

auto CursorWriter::createPrintContext() noexcept -> BlockPrintContextPtr {
    return std::make_unique<impl::BlockPrintContextToCursorWriter>(*this);
}

}
