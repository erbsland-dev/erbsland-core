// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfErrorCategory.hpp"
#include "Location.hpp"
#include "NamePath.hpp"
#include "Source_fwd.hpp"

#include "../path/Path.hpp"
#include "../text/CodeSnippet.hpp"
#include "../text/String.hpp"
#include "../unit/CodeLocation.hpp"

#include <optional>
#include <type_traits>
#include <utility>

namespace erbsland::conf {

/// Complete user-facing context for a configuration error.
/// @tested{ConfErrorTest ParserErrorClassTest}
class ConfErrorContext {
public:
    /// Create an empty internal-error context.
    ConfErrorContext() = default;
    /// Create a context with an explicit title and description.
    ConfErrorContext(ConfErrorCategory category, text::String title, text::String description) noexcept :
        _title{std::move(title)}, _description{std::move(description)}, _category{category} {}
    /// Create a context with an explicit title and source-aware location.
    ConfErrorContext(
        ConfErrorCategory category,
        text::String title,
        text::String description,
        const SourcePtr &source,
        const Location &location);
    /// Create a context with the standard title and source-aware location.
    ConfErrorContext(
        ConfErrorCategory category, text::String description, const SourcePtr &source, const Location &location);
    /// Create a context with the standard title for a category.
    template <typename... Args>
        requires((std::is_same_v<std::decay_t<Args>, Location> || std::is_same_v<std::decay_t<Args>, NamePath> ||
                     std::is_same_v<std::decay_t<Args>, path::Path>) &&
                    ...)
    ConfErrorContext(ConfErrorCategory category, text::String description, Args &&...args) noexcept :
        _title{defaultTitle(category)}, _description{std::move(description)}, _category{category} {
        (assignOptional(std::forward<Args>(args)), ...);
    }

    // defaults
    ~ConfErrorContext() = default;
    ConfErrorContext(const ConfErrorContext &) = default;
    ConfErrorContext(ConfErrorContext &&) noexcept = default;
    auto operator=(const ConfErrorContext &) -> ConfErrorContext & = default;
    auto operator=(ConfErrorContext &&) noexcept -> ConfErrorContext & = default;

public: // accessors
    /// Get the error title.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _title; }
    /// Set the error title.
    auto setTitle(text::String title) noexcept -> ConfErrorContext & {
        _title = std::move(title);
        return *this;
    }
    /// Get the error description.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _description; }
    /// Set the error description.
    auto setDescription(text::String description) noexcept -> ConfErrorContext & {
        _description = std::move(description);
        return *this;
    }
    /// Get the error category.
    [[nodiscard]] auto category() const noexcept -> ConfErrorCategory { return _category; }
    /// Set the error category.
    auto setCategory(ConfErrorCategory category) noexcept -> ConfErrorContext & {
        _category = category;
        return *this;
    }
    /// Get the optional source location.
    [[nodiscard]] auto location() const noexcept -> const std::optional<unit::CodeLocation> & { return _location; }
    /// Set the source location.
    auto setLocation(unit::CodeLocation location) noexcept -> ConfErrorContext & {
        _location = location;
        return *this;
    }
    /// Get the optional configuration name path.
    [[nodiscard]] auto namePath() const noexcept -> const std::optional<NamePath> & { return _namePath; }
    /// Set the configuration name path.
    auto setNamePath(NamePath namePath) noexcept -> ConfErrorContext & {
        _namePath = std::move(namePath);
        return *this;
    }
    /// Get the optional source file path.
    [[nodiscard]] auto filePath() const noexcept -> const std::optional<path::Path> & { return _filePath; }
    /// Set the source file path.
    auto setFilePath(path::Path filePath) noexcept -> ConfErrorContext & {
        _filePath = std::move(filePath);
        return *this;
    }
    /// Get the optional source excerpt.
    [[nodiscard]] auto codeSnippet() const noexcept -> const std::optional<text::CodeSnippet> & { return _codeSnippet; }
    /// Set the source excerpt.
    auto setCodeSnippet(text::CodeSnippet codeSnippet) noexcept -> ConfErrorContext & {
        _codeSnippet = std::move(codeSnippet);
        return *this;
    }

public: // enrichment
    /// Return a copy enriched with `location`.
    [[nodiscard]] auto withLocation(const Location &location) const -> ConfErrorContext;
    /// Return a copy enriched with a name path and location.
    [[nodiscard]] auto withNamePathAndLocation(const NamePath &namePath, const Location &location) const
        -> ConfErrorContext;
    /// Return a copy whose description has `prefix`.
    [[nodiscard]] auto withDescriptionPrefix(const text::String &prefix) const -> ConfErrorContext;
    /// Return a copy with `description`.
    [[nodiscard]] auto withDescription(text::String description) const -> ConfErrorContext;
    /// Return a copy with the optional source excerpt.
    [[nodiscard]] auto withCodeSnippet(const std::optional<text::CodeSnippet> &codeSnippet) const -> ConfErrorContext;

public:
    /// Get the standard title for an error category.
    [[nodiscard]] static auto defaultTitle(ConfErrorCategory category) noexcept -> text::String;

private:
    /// Store the optional location by copy.
    /// @param location The location to inspect.
    void assignOptional(const Location &location) noexcept;
    /// Store optional values from a temporary location.
    /// @param location The location to inspect.
    void assignOptional(Location &&location) noexcept { assignOptional(location); }
    /// Store the optional name path by copy.
    /// @param namePath The name path to store.
    void assignOptional(const NamePath &namePath) noexcept { _namePath = namePath; }
    /// Store the optional name path by move.
    /// @param namePath The name path to store.
    void assignOptional(NamePath &&namePath) noexcept { _namePath = std::move(namePath); }
    /// Store the optional file path by copy.
    /// @param filePath The file path to store.
    void assignOptional(const path::Path &filePath) noexcept { _filePath = filePath; }
    /// Store the optional file path by move.
    /// @param filePath The file path to store.
    void assignOptional(path::Path &&filePath) noexcept { _filePath = std::move(filePath); }

private:
    text::String _title;                           ///< Short summary of what failed.
    text::String _description;                     ///< Detailed reason for the failure.
    ConfErrorCategory _category;                   ///< Machine-readable error category.
    std::optional<unit::CodeLocation> _location;   ///< Optional source location.
    std::optional<NamePath> _namePath;             ///< Optional configuration name path.
    std::optional<path::Path> _filePath;           ///< Optional source file path.
    std::optional<text::CodeSnippet> _codeSnippet; ///< Optional source excerpt.
};

}
