// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionErrorDiagnostic.hpp"

#include "OptionDocumentBuilder.hpp"

#include "../../err/RuntimeError.hpp"
#include "../../i18n/DisplayText.hpp"
#include "../../text/Literals.hpp"
#include "../../text/PlainTextRenderer.hpp"

#include <utility>

namespace erbsland::options::impl {

using namespace text;
using namespace text::literals;

OptionErrorDiagnostic::OptionErrorDiagnostic(OptionErrorContext context) : _context{std::move(context)} {
}

auto OptionErrorDiagnostic::sourceName() const noexcept -> String {
    try {
        return _context.arguments().isEmpty() ? String{}
                                              : i18n::displayText("options.command_line_arguments_heading"_el);
    } catch (const err::RuntimeError &) {
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

auto OptionErrorDiagnostic::toString() const noexcept -> String {
    try {
        auto document = toTextDocument();
        return PlainTextRenderer{document}.build();
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto OptionErrorDiagnostic::toTextDocument() const -> TextDocument {

    return OptionDocumentBuilder{_context.options()}.errorDocument(_context);
}

}
