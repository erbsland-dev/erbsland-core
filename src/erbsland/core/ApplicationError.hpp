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
    /// Create an application error with the given context.
    /// @param context The context with error details.
    /// @param cause The diagnostic cause.
    explicit ApplicationError(ApplicationErrorContext context, const std::exception_ptr &cause = {}) noexcept :
        err::RuntimeError{context.title(), cause}, _context{std::move(context)} {}
    /// Create an application error with the given title, description and exit-code.
    /// @param title The title for this error (what went wrong?).
    /// @param description A description of the error (why went it wrong?)
    /// @param exitCode The exit code to use when terminating the application.
    /// @param cause The diagnostic cause.
    explicit ApplicationError(
        text::String title,
        text::String description,
        const unit::ExitCode exitCode,
        const std::exception_ptr &cause = {}) noexcept :
        err::RuntimeError{title, cause}, _context{std::move(title), std::move(description), exitCode} {}
    /// Create an application error with the given title and exit-code.
    /// @param title The title for this error (what went wrong?).
    /// @param description A description of the error (why went it wrong?)
    /// @param cause The diagnostic cause.
    explicit ApplicationError(
        text::String title, text::String description, const std::exception_ptr &cause = {}) noexcept :
        ApplicationError(std::move(title), std::move(description), unit::ExitCode::failure(), cause) {}
    /// Create an application error with the given title and exit-code.
    /// @param title The title for this error (what went wrong?).
    /// @param exitCode The exit code to use when terminating the application.
    /// @param cause The diagnostic cause.
    explicit ApplicationError(
        text::String title, const unit::ExitCode exitCode, const std::exception_ptr &cause = {}) noexcept :
        ApplicationError(std::move(title), {}, exitCode, cause) {}
    /// Create an application error with the given title and cause
    /// @param title The title for this error (what went wrong?).
    /// @param cause The diagnostic cause.
    explicit ApplicationError(text::String title, const std::exception_ptr &cause = {}) noexcept :
        ApplicationError(std::move(title), {}, unit::ExitCode::failure(), cause) {}

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

public: // enrichment
    /// Add or replace the title of the error with a new one.
    /// @param title The new title to use when missing or to replace an existing one.
    /// @param overwriteExisting Whether to overwrite an existing title. `true` replaces any existing title.
    /// @return A new instance with the updated title
    [[nodiscard]] auto withTitle(text::String title, bool overwriteExisting = false) const noexcept -> ApplicationError;
    /// Add or replace the description of the error with a new one.
    /// @param description The new description to use when missing or to replace an existing one.
    /// @param overwriteExisting Whether to overwrite an existing description. `true` replaces any existing description.
    /// @return A new instance with the updated description
    [[nodiscard]] auto withDescription(text::String description, bool overwriteExisting = false) const noexcept
        -> ApplicationError;
    /// Add or replace the name of the source that caused the error with a new one.
    /// @param sourceName The new source name to use when missing or to replace an existing one.
    /// @param overwriteExisting Whether to overwrite an existing source name. `true` replaces any existing source name.
    /// @return A new instance with the updated source name
    [[nodiscard]] auto withSourceName(text::String sourceName, bool overwriteExisting = false) const noexcept
        -> ApplicationError;
    /// Add or replace the path of the source that caused the error with a new one.
    /// @param sourcePath The new source path to use when missing or to replace an existing one.
    /// @param overwriteExisting Whether to overwrite an existing source path. `true` replaces any existing source path.
    /// @return A new instance with the updated source path
    [[nodiscard]] auto withSourcePath(text::String sourcePath, bool overwriteExisting = false) const noexcept
        -> ApplicationError;
    /// Add or replace the code location of the source that caused the error with a new one.
    /// @param codeLocation The new code location to use when missing or to replace an existing one.
    /// @param overwriteExisting Whether to overwrite an existing code location. `true` replaces any existing code
    /// location.
    /// @return A new instance with the updated code location
    [[nodiscard]] auto withCodeLocation(unit::CodeLocation codeLocation, bool overwriteExisting = false) const noexcept
        -> ApplicationError;
    /// Add or replace the exit code to end the application with a new one.
    /// @param exitCode The new exit code to use when missing or to replace an existing one.
    /// @param overwriteExisting Whether to overwrite an existing exit code. `true` replaces any existing exit code.
    /// @return A new instance with the updated exit code
    [[nodiscard]] auto withExitCode(unit::ExitCode exitCode, bool overwriteExisting = false) const noexcept
        -> ApplicationError;

private:
    ApplicationErrorContext _context;
};

}
