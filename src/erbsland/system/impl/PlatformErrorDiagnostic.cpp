// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PlatformErrorDiagnostic.hpp"

#include "../PlatformErrorContext.hpp"

#include "../../err/ErrorDocumentBuilder.hpp"
#include "../../text/TextDocument.hpp"
#include "../../text/TextNode.hpp"

#include <utility>

namespace erbsland::system::impl {

PlatformErrorDiagnostic::PlatformErrorDiagnostic(text::String title, PlatformErrorContextConstPtr context) noexcept :
    _title{std::move(title)}, _context{std::move(context)} {
}

auto PlatformErrorDiagnostic::toString() const noexcept -> text::String {
    return _title;
}

auto PlatformErrorDiagnostic::toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
    -> text::TextDocument {
    auto builder = err::ErrorDocumentBuilder{_title, {}, displayText};
    if (_context != nullptr) {
        auto contextDocument = _context->toTextDocument();
        for (const auto &child : contextDocument.root()->children()) {
            builder.root()->add(child->clone());
        }
    }
    return builder.takeDocument();
}

}
