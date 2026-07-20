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
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _title; }
    auto setTitle(text::String title) noexcept -> ConfErrorContext & {
        _title = std::move(title);
        return *this;
    }
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _description; }
    auto setDescription(text::String description) noexcept -> ConfErrorContext & {
        _description = std::move(description);
        return *this;
    }
    [[nodiscard]] auto category() const noexcept -> ConfErrorCategory { return _category; }
    auto setCategory(ConfErrorCategory category) noexcept -> ConfErrorContext & {
        _category = category;
        return *this;
    }
    [[nodiscard]] auto location() const noexcept -> const std::optional<unit::CodeLocation> & { return _location; }
    auto setLocation(unit::CodeLocation location) noexcept -> ConfErrorContext & {
        _location = location;
        return *this;
    }
    [[nodiscard]] auto namePath() const noexcept -> const std::optional<NamePath> & { return _namePath; }
    auto setNamePath(NamePath namePath) noexcept -> ConfErrorContext & {
        _namePath = std::move(namePath);
        return *this;
    }
    [[nodiscard]] auto filePath() const noexcept -> const std::optional<path::Path> & { return _filePath; }
    auto setFilePath(path::Path filePath) noexcept -> ConfErrorContext & {
        _filePath = std::move(filePath);
        return *this;
    }
    [[nodiscard]] auto codeSnippet() const noexcept -> const std::optional<text::CodeSnippet> & { return _codeSnippet; }
    auto setCodeSnippet(text::CodeSnippet codeSnippet) noexcept -> ConfErrorContext & {
        _codeSnippet = std::move(codeSnippet);
        return *this;
    }

public: // enrichment
    [[nodiscard]] auto withLocation(const Location &location) const -> ConfErrorContext;
    [[nodiscard]] auto withNamePathAndLocation(const NamePath &namePath, const Location &location) const
        -> ConfErrorContext;
    [[nodiscard]] auto withDescriptionPrefix(const text::String &prefix) const -> ConfErrorContext;
    [[nodiscard]] auto withDescription(text::String description) const -> ConfErrorContext;
    [[nodiscard]] auto withCodeSnippet(const std::optional<text::CodeSnippet> &codeSnippet) const -> ConfErrorContext;

public:
    /// Get the standard title for an error category.
    [[nodiscard]] static auto defaultTitle(ConfErrorCategory category) noexcept -> text::String;

private:
    void assignOptional(const Location &location) noexcept;
    void assignOptional(Location &&location) noexcept { assignOptional(location); }
    void assignOptional(const NamePath &namePath) noexcept { _namePath = namePath; }
    void assignOptional(NamePath &&namePath) noexcept { _namePath = std::move(namePath); }
    void assignOptional(const path::Path &filePath) noexcept { _filePath = filePath; }
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
