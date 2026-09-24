// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompressionErrorReason.hpp"

#include "../text/String.hpp"

#include <utility>

namespace erbsland::compression {

/// User-facing context for a byte-compression error.
/// @tested{CompressionErrorTest}
class CompressionErrorContext final {
public:
    /// Create context for a byte-compression error.
    /// @param reason The machine-readable failure reason.
    /// @param title A concise description of what went wrong.
    /// @param description An optional explanation of why the error occurred.
    explicit CompressionErrorContext(
        CompressionErrorReason reason, text::String title, text::String description = {}) noexcept :
        _reason{reason}, _title{std::move(title)}, _description{std::move(description)} {}

    // defaults
    ~CompressionErrorContext() = default;
    CompressionErrorContext(const CompressionErrorContext &) = default;
    CompressionErrorContext(CompressionErrorContext &&) noexcept = default;
    auto operator=(const CompressionErrorContext &) -> CompressionErrorContext & = default;
    auto operator=(CompressionErrorContext &&) noexcept -> CompressionErrorContext & = default;

public: // accessors
    /// Get the machine-readable failure reason.
    [[nodiscard]] auto reason() const noexcept -> CompressionErrorReason { return _reason; }
    /// Set the machine-readable failure reason.
    auto setReason(const CompressionErrorReason reason) noexcept -> CompressionErrorContext & {
        _reason = reason;
        return *this;
    }
    /// Get the concise error title.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _title; }
    /// Set the concise error title.
    auto setTitle(text::String title) noexcept -> CompressionErrorContext & {
        _title = std::move(title);
        return *this;
    }
    /// Get the detailed error description.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _description; }
    /// Set the detailed error description.
    auto setDescription(text::String description) noexcept -> CompressionErrorContext & {
        _description = std::move(description);
        return *this;
    }

private:
    CompressionErrorReason _reason; ///< Machine-readable failure reason.
    text::String _title;            ///< Concise error title.
    text::String _description;      ///< Detailed error description.
};

}
