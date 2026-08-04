// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BufferView.hpp"

namespace erbsland::cterm {

auto BufferView::content() const noexcept -> const ReadableBufferPtr & {
    return _content;
}

void BufferView::setContent(ReadableBufferPtr buffer) noexcept {
    _content = buffer;
}

}
