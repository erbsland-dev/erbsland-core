// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfErrorContext.hpp"

#include "../err/LogicError.hpp"
#include "../err/OutOfRangeError.hpp"
#include "../err/ParameterError.hpp"

#include <exception>
#include <utility>

namespace erbsland::conf {

/// An error raised while processing configuration data.
/// @tested{ConfErrorTest ParserErrorClassTest}
class ConfError final : public err::LogicError {
public:
    /// Create an error from its complete context.
    explicit ConfError(ConfErrorContext context, std::exception_ptr cause = {}) noexcept;
    /// Create an error with an explicit title and description.
    ConfError(ConfErrorCategory category, text::String title, text::String description, std::exception_ptr cause = {});
    /// Create an error with an explicit title and configuration location.
    ConfError(
        ConfErrorCategory category,
        text::String title,
        text::String description,
        const Location &location,
        std::exception_ptr cause = {});
    /// Create an error with an explicit title and file path.
    ConfError(
        ConfErrorCategory category,
        text::String title,
        text::String description,
        path::Path filePath,
        std::exception_ptr cause = {});
    /// Create an error with an explicit title and source-aware location.
    ConfError(
        ConfErrorCategory category,
        text::String title,
        text::String description,
        const SourcePtr &source,
        const Location &location,
        std::exception_ptr cause = {});
    /// Create an error with the standard title and source-aware location.
    ConfError(
        ConfErrorCategory category,
        text::String description,
        const SourcePtr &source,
        const Location &location,
        std::exception_ptr cause = {});
    /// Create an error with the standard title for a category.
    template <typename... Args>
        requires(
            (std::is_same_v<std::decay_t<Args>, Location> || std::is_same_v<std::decay_t<Args>, NamePath> ||
                std::is_same_v<std::decay_t<Args>, path::Path>) &&
            ...)
    ConfError(ConfErrorCategory category, text::String description, Args &&...args) noexcept :
        ConfError{ConfErrorContext{category, std::move(description), std::forward<Args>(args)...}} {}

    // defaults
    ~ConfError() override = default;
    ConfError(const ConfError &) = default;
    ConfError(ConfError &&) noexcept = default;
    auto operator=(const ConfError &) -> ConfError & = default;
    auto operator=(ConfError &&) noexcept -> ConfError & = default;

public: // implement LogicError
    [[nodiscard]] auto diagnostic() const -> err::DiagnosticConstPtr override;

public: // accessors
    /// Access the complete error context.
    [[nodiscard]] auto context() const noexcept -> const ConfErrorContext & { return _context; }
    /// Access the error category.
    [[nodiscard]] auto category() const noexcept -> ConfErrorCategory { return _context.category(); }
    /// Access the diagnostic title.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _context.title(); }
    /// Access the diagnostic description.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _context.description(); }
    /// Access the optional diagnostic location.
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation {
        return _context.location().value_or(unit::CodeLocation{});
    }
    /// Access the optional diagnostic name path.
    [[nodiscard]] auto namePath() const noexcept -> NamePath { return _context.namePath().value_or(NamePath{}); }
    /// Access the optional diagnostic file path.
    [[nodiscard]] auto filePath() const noexcept -> path::Path { return _context.filePath().value_or(path::Path{}); }

public: // enrichment
    /// Return this error with a replacement location.
    [[nodiscard]] auto withLocation(const Location &location) const -> ConfError;
    /// Return this error with a replacement name path and location.
    [[nodiscard]] auto withNamePathAndLocation(const NamePath &namePath, const Location &location) const -> ConfError;
    /// Return this error with a description prefix.
    [[nodiscard]] auto withDescriptionPrefix(const text::String &prefix) const -> ConfError;
    /// Return this error with a replacement description.
    [[nodiscard]] auto withDescription(text::String description) const -> ConfError;
    /// Return this error with a replacement code snippet.
    [[nodiscard]] auto withCodeSnippet(const std::optional<text::CodeSnippet> &codeSnippet) const -> ConfError;

private:
    /// Return this error with a replacement complete context.
    [[nodiscard]] auto withContext(ConfErrorContext context) const -> ConfError;

private:
    ConfErrorContext _context; ///< The complete diagnostic context.
};

}
