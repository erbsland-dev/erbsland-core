// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegExError.hpp"

#include "impl/error/RegExErrorDiagnostic.hpp"

#include "../text/StringFormat.hpp"

#include <memory>
#include <utility>

namespace erbsland::re {

using namespace text::literals;

RegExError::RegExError(const ErrorCategory category, text::StringView title) noexcept :
    RegExError{RegExErrorContext{category, std::move(title)}} {
}

RegExError::RegExError(const ErrorCategory category, text::StringView title, const unit::CodeLocation location) noexcept
    :
    RegExError{RegExErrorContext{category, std::move(title), {}, location}} {
}

RegExError::RegExError(
    const ErrorCategory category,
    text::StringView title,
    text::StringView description,
    const unit::CodeLocation location) noexcept :
    RegExError{RegExErrorContext{category, std::move(title), std::move(description), location}} {
}

RegExError::RegExError(RegExErrorContext context) noexcept :
    err::RuntimeError{context.title()}, _context{std::move(context)} {
}

auto RegExError::toString() const noexcept -> text::StringView {
    if (description().isEmpty()) {
        return text::StringFormat{"{}: {}"_el}.build(re::toString(category()), title());
    }
    return text::StringFormat{"{}: {}. {}"_el}.build(re::toString(category()), title(), description());
}

auto RegExError::diagnostic() const -> err::DiagnosticConstPtr {
    return std::make_shared<impl::RegExErrorDiagnostic>(_context);
}

auto RegExError::withLineNumber(const unit::LineIndex lineNumber) const noexcept -> RegExError {
    auto context = _context;
    auto location = context.location();
    location.line = lineNumber;
    context.setLocation(location);
    return RegExError{std::move(context)};
}

}
