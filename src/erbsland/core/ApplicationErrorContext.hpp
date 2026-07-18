// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"
#include "../unit/CodeLocation.hpp"
#include "../unit/ExitCode.hpp"

namespace erbsland::core {

/// Generic additional data to extend application reporting.
/// @tested{ApplicationErrorTest}
class ApplicationErrorContext {
public:
    /// Create an error context with details for error reporting
    /// @param title The title for the error, which is also the error `reason`.
    /// @param description A detailed description of the error.
    /// @param exitCode The exit code if this exception ends the current application.
    ApplicationErrorContext(
        text::String title, text::String description, const unit::ExitCode exitCode = unit::ExitCode::failure()) :
        _title{std::move(title)}, _description{std::move(description)}, _exitCode{exitCode} {}
    /// @overload
    ApplicationErrorContext( // NOLINT(*-explicit-constructor)
        text::String title,
        const unit::ExitCode exitCode = unit::ExitCode::failure()) :
        ApplicationErrorContext(std::move(title), {}, exitCode) {}
    /// @overload
    ApplicationErrorContext( // NOLINT(*-explicit-constructor)
        const unit::ExitCode exitCode = unit::ExitCode::failure()) :
        ApplicationErrorContext(text::String{}, exitCode) {}

public:
    /// Get the title of the error.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _title; }
    /// Set the title of the error.
    auto setTitle(text::String title) noexcept -> ApplicationErrorContext & {
        _title = std::move(title);
        return *this;
    }
    /// Get the description of the error.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _description; }
    /// Set the description of the error.
    auto setDescription(text::String description) noexcept -> ApplicationErrorContext & {
        _description = std::move(description);
        return *this;
    }
    /// Get the name of the source that caused the error.
    [[nodiscard]] auto sourceName() const noexcept -> const text::String & { return _sourceName; }
    /// Set the name of the source that caused the error.
    auto setSourceName(text::String sourceName) noexcept -> ApplicationErrorContext & {
        _sourceName = std::move(sourceName);
        return *this;
    }
    /// Get the path of the source that caused the error.
    [[nodiscard]] auto sourcePath() const noexcept -> const text::String & { return _sourcePath; }
    /// Set the path of the source that caused the error.
    auto setSourcePath(text::String sourcePath) noexcept -> ApplicationErrorContext & {
        _sourcePath = std::move(sourcePath);
        return *this;
    }
    /// Get the code location of the source that caused the error.
    [[nodiscard]] auto codeLocation() const noexcept -> unit::CodeLocation { return _codeLocation; }
    /// Set the code location of the source that caused the error.
    auto setCodeLocation(const unit::CodeLocation codeLocation) noexcept -> ApplicationErrorContext & {
        _codeLocation = codeLocation;
        return *this;
    }
    /// Get the exit code to end the application.
    [[nodiscard]] auto exitCode() const noexcept -> unit::ExitCode { return _exitCode; }
    /// Set the exit code to end the application.
    auto setExitCode(const unit::ExitCode exitCode) noexcept -> ApplicationErrorContext & {
        _exitCode = exitCode;
        return *this;
    }

private:
    text::String _title;              ///< The error title or reason.
    text::String _description;        ///< The detailed error description.
    text::String _sourceName;         ///< The name of the source.
    text::String _sourcePath;         ///< A path to the source.
    unit::CodeLocation _codeLocation; ///< The location in code.
    unit::ExitCode _exitCode;
};

}
