// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionErrorDiagnostic.hpp"

#include "OptionDocumentBuilder.hpp"

#include "../../i18n/DisplayTextMap.hpp"
#include "../../text/Literals.hpp"
#include "../../text/PlainTextRenderer.hpp"

#include <utility>

namespace erbsland::options::impl {

using namespace text::literals;

OptionErrorDiagnostic::OptionErrorDiagnostic(OptionErrorContext context) : _context{std::move(context)} {
}

auto OptionErrorDiagnostic::sourceName() const noexcept -> text::String {
    try {
        return _context.arguments().isEmpty() || _context.displayText() == nullptr
            ? text::String{}
            : _context.displayText()->text("options.CommandLineArgumentsHeading"_el);
    } catch (...) {
        return {};
    }
}

auto OptionErrorDiagnostic::location() const noexcept -> unit::CodeLocation {
    auto result = unit::CodeLocation{};
    if (!_context.argumentIndex().isNoIndex()) {
        result.setLine(unit::LineIndex::fromSizeT(_context.argumentIndex().toSizeT()));
    }
    return result;
}

auto OptionErrorDiagnostic::toString() const noexcept -> text::String {
    try {
        auto document = toTextDocument();
        return text::PlainTextRenderer{document}.build();
    } catch (...) {
        return {};
    }
}

auto OptionErrorDiagnostic::toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
    -> text::TextDocument {
    const auto resolvedDisplayText = displayText != nullptr ? displayText : _context.displayText();
    return OptionDocumentBuilder{_context.options(), resolvedDisplayText}.errorDocument(_context);
}

}
