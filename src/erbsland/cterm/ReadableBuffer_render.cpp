// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReadableBuffer.hpp"

#include "impl/BufferRenderer.hpp"

namespace erbsland::cterm {

auto ReadableBuffer::toString(const BufferRenderOptions &options) const -> text::String {
    return impl::BufferRenderer{*this, options}.render();
}

}
