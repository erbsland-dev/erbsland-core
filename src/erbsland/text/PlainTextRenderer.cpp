// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PlainTextRenderer.hpp"

#include "impl/PlainTextRenderer.hpp"

#include <memory>

namespace erbsland::text {

PlainTextRenderer::PlainTextRenderer(const TextDocument &document) :
    _impl{std::make_unique<impl::PlainTextRenderer>(document)} {
}

PlainTextRenderer::~PlainTextRenderer() = default;

auto PlainTextRenderer::build() -> String {
    return _impl->build();
}

auto PlainTextRenderer::appendTo(AnyStringBuilder &builder) -> AnyStringBuilder & {
    return _impl->appendTo(builder);
}

}
