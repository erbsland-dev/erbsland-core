// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegExErrorDiagnostic.hpp"

#include "../../../err/ErrorDocumentBuilder.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/PlainTextRenderer.hpp"
#include "../../../text/TextDocument.hpp"
#include "../../../text/TextNode.hpp"
#include "../../../text/TextNodeType.hpp"
#include "../../ErrorCategory.hpp"

#include <utility>

namespace erbsland::re::impl {

using namespace text::literals;

RegExErrorDiagnostic::RegExErrorDiagnostic(RegExErrorContext context) noexcept : _context{std::move(context)} {
}

auto RegExErrorDiagnostic::location() const noexcept -> unit::CodeLocation {
    return _context.location();
}

auto RegExErrorDiagnostic::toString() const noexcept -> text::String {
    try {
        return text::PlainTextRenderer{toTextDocument({})}.build();
    } catch (...) {
        return _context.title();
    }
}

auto RegExErrorDiagnostic::toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const -> text::TextDocument {
    auto builder = err::ErrorDocumentBuilder{_context.title(), _context.description(), displayText};
    builder.addSource({}, {}, _context.location());
    builder.addSection("Details"_el);
    auto list = builder.root()->add(text::TextNodeType::FieldList);
    auto item = list->add(text::TextNodeType::FieldItem);
    item->add(text::TextNodeType::FieldLabel)->addText("category"_el);
    item->add(text::TextNodeType::FieldContent)->addText(re::toString(_context.category()));
    return builder.takeDocument();
}

}
