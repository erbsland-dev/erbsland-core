// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationErrorContext.hpp"

#include "../err/RuntimeError.hpp"
#include "../text/String.hpp"
#include "../unit/ExitCode.hpp"

namespace erbsland::core {

/// A generic application error that is meant to terminate a running application with the given message and exit code.
/// @tested{ApplicationErrorTest}
class ApplicationError : public err::RuntimeError {
public:
    /// Create an application error with the given reason and exit-code.
    /// @param reason The reason for the error.
    /// @param cause The diagnostic cause.
    /// @param exitCode The exit code to use when terminating the application.
    explicit ApplicationError(
        text::String reason,
        const std::exception_ptr &cause,
        const unit::ExitCode exitCode = unit::ExitCode::failure()) noexcept :
        err::RuntimeError{reason, cause}, _context{std::move(reason), exitCode} {}
    /// Create an application error with the given reason and exit-code.
    /// @param context The context with error details.
    /// @param cause The diagnostic cause.
    explicit ApplicationError(ApplicationErrorContext context, const std::exception_ptr &cause = {}) noexcept :
        err::RuntimeError{context.title(), cause}, _context{std::move(context)} {}
    /// @overload
    explicit ApplicationError(text::String reason, const unit::ExitCode exitCode = unit::ExitCode::failure()) noexcept :
        ApplicationError(std::move(reason), {}, exitCode) {}

    // defaults
    ~ApplicationError() override = default;

public: // overrides
    [[nodiscard]] auto diagnostic() const -> err::DiagnosticConstPtr override;

public: // accessors
    /// Get the title of the error.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _context.title(); }
    /// Get the description of the error.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _context.description(); }
    /// Get the name of the source that caused the error.
    [[nodiscard]] auto sourceName() const noexcept -> const text::String & { return _context.sourceName(); }
    /// Get the path of the source that caused the error.
    [[nodiscard]] auto sourcePath() const noexcept -> const text::String & { return _context.sourcePath(); }
    /// Get the code location of the source that caused the error.
    [[nodiscard]] auto codeLocation() const noexcept -> unit::CodeLocation { return _context.codeLocation(); }
    /// Get the exit code to end the application.
    [[nodiscard]] auto exitCode() const noexcept -> unit::ExitCode { return _context.exitCode(); }
    /// Get the error context.
    [[nodiscard]] auto context() const noexcept -> const ApplicationErrorContext & { return _context; }

private:
    ApplicationErrorContext _context;
};

}
