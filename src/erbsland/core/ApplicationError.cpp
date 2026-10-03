// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationError.hpp"

#include "impl/ApplicationErrorDiagnostic.hpp"

#include "../text/StringFormat.hpp"

namespace erbsland::core {

using namespace text;

auto ApplicationError::diagnostic() const -> err::DiagnosticConstPtr {
    return std::make_shared<impl::ApplicationErrorDiagnostic>(_context);
}

auto ApplicationError::withTitle(String title, const bool overwriteExisting) const noexcept -> ApplicationError {
    if (!overwriteExisting && !_context.title().isEmpty()) {
        return *this;
    }
    auto context = _context;
    context.setTitle(std::move(title));
    return ApplicationError{context, cause()};
}

auto ApplicationError::withDescription(String description, const bool overwriteExisting) const noexcept
    -> ApplicationError {
    if (!overwriteExisting && !_context.description().isEmpty()) {
        return *this;
    }
    auto context = _context;
    context.setDescription(std::move(description));
    return ApplicationError{context, cause()};
}

auto ApplicationError::withSourceName(String sourceName, const bool overwriteExisting) const noexcept
    -> ApplicationError {
    if (!overwriteExisting && !_context.sourceName().isEmpty()) {
        return *this;
    }
    auto context = _context;
    context.setSourceName(std::move(sourceName));
    return ApplicationError{context, cause()};
}

auto ApplicationError::withSourcePath(String sourcePath, const bool overwriteExisting) const noexcept
    -> ApplicationError {
    if (!overwriteExisting && !_context.sourcePath().isEmpty()) {
        return *this;
    }
    auto context = _context;
    context.setSourcePath(std::move(sourcePath));
    return ApplicationError{context, cause()};
}

auto ApplicationError::withCodeLocation(
    const unit::CodeLocation codeLocation, const bool overwriteExisting) const noexcept -> ApplicationError {
    if (!overwriteExisting && !_context.codeLocation().isUndefined()) {
        return *this;
    }
    auto context = _context;
    context.setCodeLocation(codeLocation);
    return ApplicationError{context, cause()};
}

auto ApplicationError::withExitCode(const unit::ExitCode exitCode, const bool overwriteExisting) const noexcept
    -> ApplicationError {
    if (!overwriteExisting && _context.exitCode() != unit::ExitCode::failure()) {
        return *this;
    }
    auto context = _context;
    context.setExitCode(exitCode);
    return ApplicationError{context, cause()};
}

}
