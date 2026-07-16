// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ErrorCategory.hpp"

#include "../text/StringView.hpp"
#include "../unit/CodeLocation.hpp"

#include <utility>

namespace erbsland::re {

/// Context for a regular-expression error.
/// @tested{RegExErrorTest}
class RegExErrorContext final {
public:
    /// Create context for a regular-expression error.
    /// @param category The error category.
    /// @param title A concise description of what went wrong.
    /// @param description An optional explanation of why the error occurred.
    /// @param location The optional source location.
    explicit RegExErrorContext(
        ErrorCategory category,
        text::StringView title,
        text::StringView description = {},
        unit::CodeLocation location = {}) noexcept :
        _category{category}, _title{std::move(title)}, _description{std::move(description)}, _location{location} {}

    // defaults
    ~RegExErrorContext() = default;
    RegExErrorContext(const RegExErrorContext &) = default;
    RegExErrorContext(RegExErrorContext &&) noexcept = default;
    auto operator=(const RegExErrorContext &) -> RegExErrorContext & = default;
    auto operator=(RegExErrorContext &&) noexcept -> RegExErrorContext & = default;

public: // accessors
    /// Get the error category.
    [[nodiscard]] auto category() const noexcept -> ErrorCategory { return _category; }
    /// Set the error category.
    auto setCategory(const ErrorCategory category) noexcept -> RegExErrorContext & {
        _category = category;
        return *this;
    }
    /// Get the concise error title.
    [[nodiscard]] auto title() const noexcept -> const text::StringView & { return _title; }
    /// Set the concise error title.
    auto setTitle(text::StringView title) noexcept -> RegExErrorContext & {
        _title = std::move(title);
        return *this;
    }
    /// Get the detailed error description.
    [[nodiscard]] auto description() const noexcept -> const text::StringView & { return _description; }
    /// Set the detailed error description.
    auto setDescription(text::StringView description) noexcept -> RegExErrorContext & {
        _description = std::move(description);
        return *this;
    }
    /// Get the source location.
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation { return _location; }
    /// Set the source location.
    auto setLocation(const unit::CodeLocation location) noexcept -> RegExErrorContext & {
        _location = location;
        return *this;
    }
    /// Get the source line index.
    [[nodiscard]] auto line() const noexcept -> unit::LineIndex { return _location.line; }
    /// Get the source column index.
    [[nodiscard]] auto column() const noexcept -> unit::ColumnIndex { return _location.column; }
    /// Get the source code-point position.
    [[nodiscard]] auto position() const noexcept -> unit::CpIndex { return _location.position; }

private:
    ErrorCategory _category{ErrorCategory::Internal}; ///< The error category.
    text::StringView _title;                          ///< Concise error title.
    text::StringView _description;                    ///< Detailed error description.
    unit::CodeLocation _location;                     ///< Optional source location.
};

}
