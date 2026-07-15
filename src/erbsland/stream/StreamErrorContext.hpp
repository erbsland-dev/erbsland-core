// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../system/PlatformErrorContext_fwd.hpp"
#include "../text/StringView.hpp"

namespace erbsland::stream {

/// User-facing context for a stream-domain error.
/// @tested{DiagnosticTest}
class StreamErrorContext final {
public:
    /// Create context for a failed stream operation.
    /// @param title A short developer-authored title.
    /// @param description A developer-authored explanation of the failure.
    explicit StreamErrorContext(text::StringView title, text::StringView description) noexcept;

    // defaults
    StreamErrorContext(const StreamErrorContext &) = default;
    StreamErrorContext(StreamErrorContext &&) noexcept = default;
    auto operator=(const StreamErrorContext &) -> StreamErrorContext & = default;
    auto operator=(StreamErrorContext &&) noexcept -> StreamErrorContext & = default;

public: // accessors
    /// Get the operation title.
    [[nodiscard]] auto title() const noexcept -> const text::StringView & { return _title; }
    /// Set the operation title.
    auto setTitle(text::StringView title) noexcept -> StreamErrorContext &;
    /// Get the operation description.
    [[nodiscard]] auto description() const noexcept -> const text::StringView & { return _description; }
    /// Set the operation description.
    auto setDescription(text::StringView description) noexcept -> StreamErrorContext &;
    /// Get explicit help or category-derived help when available.
    [[nodiscard]] auto help() const noexcept -> text::StringView;
    /// Set explicit help, overriding category-derived help.
    auto setHelp(text::StringView help) noexcept -> StreamErrorContext &;
    /// Get the stream path, or an empty view if none was provided.
    [[nodiscard]] auto path() const noexcept -> const text::StringView & { return _path; }
    /// Set the stream path.
    auto setPath(text::StringView path) noexcept -> StreamErrorContext &;
    /// Get the immutable native failure context, if available.
    [[nodiscard]] auto platformContext() const noexcept -> const system::PlatformErrorContextConstPtr & {
        return _platformContext;
    }
    /// Set the immutable native failure context.
    auto setPlatformContext(system::PlatformErrorContextConstPtr platformContext) noexcept -> StreamErrorContext &;

private:
    [[nodiscard]] auto categoryHelp() const noexcept -> text::StringView;

private:
    text::StringView _title;                               ///< Short operation title.
    text::StringView _description;                         ///< Detailed operation description.
    text::StringView _help;                                ///< Explicit recovery guidance.
    text::StringView _path;                                ///< Stream path, when available.
    system::PlatformErrorContextConstPtr _platformContext; ///< Embedded native failure details.
};

}
