// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CompressionErrorDiagnostic.hpp"

#include "../../err/ErrorDocumentBuilder.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../text/Literals.hpp"
#include "../../text/PlainTextRenderer.hpp"
#include "../../text/TextDocument.hpp"
#include "../../text/TextNode.hpp"
#include "../../text/TextNodeType.hpp"

#include <utility>

namespace erbsland::compression::impl {

using namespace text;
using namespace text::literals;

CompressionErrorDiagnostic::CompressionErrorDiagnostic(CompressionErrorContext context) noexcept :
    _context{std::move(context)} {
}

auto CompressionErrorDiagnostic::toString() const noexcept -> String {
    try {
        return PlainTextRenderer{toTextDocument()}.build();
    } catch (const err::RuntimeError &) {
        return _context.title();
    }
}

auto CompressionErrorDiagnostic::toTextDocument() const -> TextDocument {
    auto builder = err::ErrorDocumentBuilder{_context.title(), _context.description()};
    builder.addSection("Details"_el);
    auto list = builder.root()->add(TextNodeType::FieldList);
    auto item = list->add(TextNodeType::FieldItem);
    item->add(TextNodeType::FieldLabel)->addText("reason"_el);
    item->add(TextNodeType::FieldContent)->addText(reasonName(_context.reason()));
    return builder.takeDocument();
}

auto CompressionErrorDiagnostic::reasonName(const CompressionErrorReason reason) noexcept -> String {
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
