// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegExErrorDiagnostic.hpp"

#include "../../../err/ErrorDocumentBuilder.hpp"
#include "../../../err/RuntimeError.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/PlainTextRenderer.hpp"
#include "../../../text/TextDocument.hpp"
#include "../../../text/TextNode.hpp"
#include "../../../text/TextNodeType.hpp"
#include "../../ErrorCategory.hpp"

#include <utility>

namespace erbsland::re::impl {

using namespace text;
using namespace text::literals;

RegExErrorDiagnostic::RegExErrorDiagnostic(RegExErrorContext context) noexcept : _context{std::move(context)} {
}

auto RegExErrorDiagnostic::location() const noexcept -> unit::CodeLocation {
    return _context.location();
}

auto RegExErrorDiagnostic::toString() const noexcept -> String {
    try {
        return PlainTextRenderer{toTextDocument()}.build();
    } catch (const err::RuntimeError &) {
        return _context.title();
    }
}

auto RegExErrorDiagnostic::toTextDocument() const -> TextDocument {
    auto builder = err::ErrorDocumentBuilder{_context.title(), _context.description()};
    builder.addSource({}, {}, _context.location());
    builder.addSection("Details"_el);
    auto list = builder.root()->add(TextNodeType::FieldList);
    auto item = list->add(TextNodeType::FieldItem);
    item->add(TextNodeType::FieldLabel)->addText("category"_el);
    item->add(TextNodeType::FieldContent)->addText(re::toString(_context.category()));
    return builder.takeDocument();
}

}
