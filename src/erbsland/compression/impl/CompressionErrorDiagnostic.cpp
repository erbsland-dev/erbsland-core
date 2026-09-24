// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CompressionErrorDiagnostic.hpp"

#include "../../err/ErrorDocumentBuilder.hpp"
#include "../../text/Literals.hpp"
#include "../../text/PlainTextRenderer.hpp"
#include "../../text/TextDocument.hpp"
#include "../../text/TextNode.hpp"
#include "../../text/TextNodeType.hpp"

#include <utility>

namespace erbsland::compression::impl {

using namespace text::literals;

CompressionErrorDiagnostic::CompressionErrorDiagnostic(CompressionErrorContext context) noexcept :
    _context{std::move(context)} {
}

auto CompressionErrorDiagnostic::toString() const noexcept -> text::String {
    try {
        return text::PlainTextRenderer{toTextDocument({})}.build();
    } catch (...) {
        return _context.title();
    }
}

auto CompressionErrorDiagnostic::toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
    -> text::TextDocument {
    auto builder = err::ErrorDocumentBuilder{_context.title(), _context.description(), displayText};
    builder.addSection("Details"_el);
    auto list = builder.root()->add(text::TextNodeType::FieldList);
    auto item = list->add(text::TextNodeType::FieldItem);
    item->add(text::TextNodeType::FieldLabel)->addText("reason"_el);
    item->add(text::TextNodeType::FieldContent)->addText(reasonName(_context.reason()));
    return builder.takeDocument();
}

auto CompressionErrorDiagnostic::reasonName(const CompressionErrorReason reason) noexcept -> text::String {
    switch (reason) {
    case CompressionErrorReason::Cancelled:
        return "Cancelled"_el;
    case CompressionErrorReason::Timeout:
        return "Timeout"_el;
    case CompressionErrorReason::MalformedData:
        return "Malformed data"_el;
    case CompressionErrorReason::LengthMismatch:
        return "Length mismatch"_el;
    case CompressionErrorReason::UnsupportedAlgorithm:
        return "Unsupported algorithm"_el;
    case CompressionErrorReason::AlgorithmMismatch:
        return "Algorithm mismatch"_el;
    case CompressionErrorReason::UnsupportedFeature:
        return "Unsupported feature"_el;
    case CompressionErrorReason::UnsupportedEnvelopeVersion:
        return "Unsupported envelope version"_el;
    }
    return "Unknown"_el;
}

}
