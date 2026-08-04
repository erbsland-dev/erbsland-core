// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NetworkErrorDiagnostic.hpp"

#include "../../err/ErrorDocumentBuilder.hpp"
#include "../../i18n/DisplayTextMap.hpp"
#include "../../system/PlatformErrorContext.hpp"
#include "../../text/EscapeFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/TextDocument.hpp"
#include "../../text/TextNode.hpp"
#include "../../text/TextNodeType.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

NetworkErrorDiagnostic::NetworkErrorDiagnostic(NetworkErrorContext context) noexcept : _context{std::move(context)} {
}

auto NetworkErrorDiagnostic::toString() const noexcept -> text::String {
    return _context.title();
}

auto NetworkErrorDiagnostic::toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
    -> text::TextDocument {
    const auto resolvedDisplayText = displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap();
    auto builder = err::ErrorDocumentBuilder{_context.title(), _context.description(), resolvedDisplayText};
    if (_context.host().has_value() || _context.localEndpoint().has_value() || _context.remoteEndpoint().has_value()) {
        builder.addSection("Network values"_el);
        auto list = builder.root()->add(text::TextNodeType::FieldList);
        const auto addField = [&list](const text::String &label, const text::String &value) -> void {
            auto item = list->add(text::TextNodeType::FieldItem);
            item->add(text::TextNodeType::FieldLabel)->addText(label);
            item->add(text::TextNodeType::FieldContent)->addEscapedText(value, text::EscapeFormat::Display);
        };
        if (_context.host().has_value()) {
            addField("host"_el, _context.host()->toString());
        }
        if (_context.localEndpoint().has_value()) {
            addField("local endpoint"_el, _context.localEndpoint()->toString());
        }
        if (_context.remoteEndpoint().has_value()) {
            addField("remote endpoint"_el, _context.remoteEndpoint()->toString());
        }
    }
    if (_context.platformContext() != nullptr) {
        builder.addSection(resolvedDisplayText->text("PlatformErrorHeading"_el));
        const auto platformDocument = _context.platformContext()->toTextDocument();
        for (const auto &child : platformDocument.root()->children()) {
            builder.root()->add(child->clone());
        }
    }
    return builder.takeDocument();
}

}
