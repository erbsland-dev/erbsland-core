// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RenderErrorCategory.hpp"

#include "../CodeSnippet.hpp"
#include "../String.hpp"
#include "../StringList.hpp"

#include "../../unit/CodeLocation.hpp"

namespace erbsland::text::render {

/// Complete user-facing context for a render error.
/// @tested{RenderEnvironmentTest}
class RenderErrorContext final {
public:
    /// Create a new error context with a title and description.
    RenderErrorContext(RenderErrorCategory category, String title, String description) noexcept :
        _category{category}, _title{std::move(title)}, _description{std::move(description)} {}

public: // accessors
    /// Access the error category.
    [[nodiscard]] auto category() const noexcept -> RenderErrorCategory { return _category; }
    /// The title of the error. *What* went wrong.
    [[nodiscard]] auto title() const noexcept -> const String & { return _title; }
    /// The description of the error. *Why* it went wrong.
    [[nodiscard]] auto description() const noexcept -> const String & { return _description; }
    /// Access the logical layout name.
    [[nodiscard]] auto layout() const noexcept -> const String & { return _layout; }
    /// Set the logical layout name.
    auto setLayout(String layout) noexcept -> RenderErrorContext & {
        _layout = std::move(layout);
        return *this;
    }
    /// Access the optional source origin.
    [[nodiscard]] auto origin() const noexcept -> const String & { return _origin; }
    /// Set the source origin.
    auto setOrigin(String origin) noexcept -> RenderErrorContext & {
        _origin = std::move(origin);
        return *this;
    }
    /// An optional location in the file/resource that caused the error.
    [[nodiscard]] auto location() const noexcept -> const unit::CodeLocation & { return _location; }
    /// Set the location in the file/resource that caused the error.
    auto setLocation(const unit::CodeLocation location) noexcept -> RenderErrorContext & {
        _location = location;
        return *this;
    }
    /// Access the optional source snippet.
    [[nodiscard]] auto codeSnippet() const noexcept -> const std::optional<CodeSnippet> & { return _codeSnippet; }
    /// Set the source snippet.
    auto setCodeSnippet(CodeSnippet codeSnippet) noexcept -> RenderErrorContext & {
        _codeSnippet = std::move(codeSnippet);
        return *this;
    }
    /// Access the nested layout frames, outermost first.
    [[nodiscard]] auto frames() const noexcept -> const StringList & { return _frames; }
    /// Append a nested layout frame.
    auto addFrame(String frame) -> RenderErrorContext & {
        _frames.append(std::move(frame));
        return *this;
    }
    /// Prepend an outer nested layout frame.
    auto addOuterFrame(String frame) -> RenderErrorContext & {
        _frames.prepend(std::move(frame));
        return *this;
    }

private:
    RenderErrorCategory _category;           ///< Machine-readable error category.
    String _title;                           ///< Short title describing what went wrong.
    String _description;                     ///< Detailed reason for the failure.
    String _layout;                          ///< Logical layout name, if known.
    String _origin;                          ///< Source origin, if known.
    unit::CodeLocation _location;            ///< Source location, if known.
    std::optional<CodeSnippet> _codeSnippet; ///< Optional source excerpt.
    StringList _frames;                      ///< Nested render frames.
};

}
