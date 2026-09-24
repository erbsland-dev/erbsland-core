// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfError.hpp"

#include "Value.hpp"

#include "impl/ConfErrorDiagnostic.hpp"

#include <memory>
#include <utility>

namespace erbsland::conf {

ConfError::ConfError(ConfErrorContext context, const std::exception_ptr &cause) noexcept :
    err::RuntimeError{context.title(), cause}, _context{std::move(context)} {
}

ConfError::ConfError(
    const ConfErrorCategory category, text::String title, text::String description, const std::exception_ptr &cause) :
    ConfError{ConfErrorContext{category, std::move(title), std::move(description)}, cause} {
}

ConfError::ConfError(
    const ConfErrorCategory category,
    text::String title,
    text::String description,
    const Location &location,
    const std::exception_ptr &cause) :
    ConfError{ConfErrorContext{category, std::move(title), std::move(description)}.withLocation(location), cause} {
}

ConfError::ConfError(
    const ConfErrorCategory category,
    text::String title,
    text::String description,
    path::Path filePath,
    const std::exception_ptr &cause) :
    ConfError{
        ConfErrorContext{category, std::move(title), std::move(description)}.setFilePath(std::move(filePath)), cause} {
}

ConfError::ConfError(
    const ConfErrorCategory category,
    text::String title,
    text::String description,
    const SourcePtr &source,
    const Location &location,
    const std::exception_ptr &cause) :
    ConfError{ConfErrorContext{category, std::move(title), std::move(description), source, location}, cause} {
}

ConfError::ConfError(
    const ConfErrorCategory category,
    text::String description,
    const SourcePtr &source,
    const Location &location,
    const std::exception_ptr &cause) :
    ConfError{ConfErrorContext{category, std::move(description), source, location}, cause} {
}

ConfError::ConfError(
    const ValuePtr &value, text::String title, text::String description, const std::exception_ptr &cause) :
    ConfError(ConfErrorCategory::Validation, std::move(title), std::move(description), value->location(), cause) {
}

ConfError::ConfError(const ValuePtr &value, text::String description, const std::exception_ptr &cause) :
    ConfError(ConfErrorCategory::Validation, {}, std::move(description), value->location(), cause) {
}

auto ConfError::diagnostic() const -> err::DiagnosticConstPtr {
    return std::make_shared<impl::ConfErrorDiagnostic>(_context);
}

auto ConfError::withContext(ConfErrorContext context) const -> ConfError {
    return ConfError{std::move(context), cause()};
}

auto ConfError::withLocation(const Location &location) const -> ConfError {
    return withContext(_context.withLocation(location));
}

auto ConfError::withNamePathAndLocation(const NamePath &namePath, const Location &location) const -> ConfError {
    return withContext(_context.withNamePathAndLocation(namePath, location));
}

auto ConfError::withDescriptionPrefix(const text::String &prefix) const -> ConfError {
    return withContext(_context.withDescriptionPrefix(prefix));
}

auto ConfError::withDescription(text::String description) const -> ConfError {
    return withContext(_context.withDescription(std::move(description)));
}

auto ConfError::withCodeSnippet(const std::optional<text::CodeSnippet> &codeSnippet) const -> ConfError {
    return withContext(_context.withCodeSnippet(codeSnippet));
}

auto ConfError::withoutCodeSnippet() const -> ConfError {
    return withContext(_context.withoutCodeSnippet());
}

}
