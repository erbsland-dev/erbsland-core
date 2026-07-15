// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationErrorDocumentBuilder.hpp"

#include "../../err/ErrorDocumentBuilder.hpp"

namespace erbsland::core::impl {

auto ApplicationErrorDocumentBuilder::build() const -> text::TextDocument {
    auto builder = err::ErrorDocumentBuilder{_context.title(), _context.description(), _displayText};
    builder.addSource(_context.sourceName(), _context.sourcePath(), _context.codeLocation());
    return builder.takeDocument();
}

}
