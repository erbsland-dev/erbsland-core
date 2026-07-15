// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationErrorDiagnostic.hpp"

#include "ApplicationErrorDocumentBuilder.hpp"

#include "../../text/TextDocument.hpp"

#include <utility>

namespace erbsland::core::impl {

ApplicationErrorDiagnostic::ApplicationErrorDiagnostic(ApplicationErrorContext context) noexcept :
    _context{std::move(context)} {
}

auto ApplicationErrorDiagnostic::sourceName() const noexcept -> text::StringView {
    return _context.sourceName();
}

auto ApplicationErrorDiagnostic::sourcePath() const noexcept -> text::StringView {
    return _context.sourcePath();
}

auto ApplicationErrorDiagnostic::location() const noexcept -> unit::CodeLocation {
    return _context.codeLocation();
}

auto ApplicationErrorDiagnostic::toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
    -> text::TextDocument {
    return ApplicationErrorDocumentBuilder{_context, displayText}.build();
}

}
