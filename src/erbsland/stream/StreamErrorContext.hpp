// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../system/PlatformErrorContext_fwd.hpp"
#include "../text/String.hpp"

namespace erbsland::stream {

/// User-facing context for a stream-domain error.
/// @tested{DiagnosticTest}
class StreamErrorContext final {
public:
    /// Create context for a failed stream operation.
    /// @param title A short developer-authored title.
    /// @param description A developer-authored explanation of the failure.
    explicit StreamErrorContext(text::String title, text::String description) noexcept;

    // defaults
    StreamErrorContext(const StreamErrorContext &) = default;
    StreamErrorContext(StreamErrorContext &&) noexcept = default;
    auto operator=(const StreamErrorContext &) -> StreamErrorContext & = default;
    auto operator=(StreamErrorContext &&) noexcept -> StreamErrorContext & = default;

public: // accessors
    /// Get the operation title.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _title; }
    /// Set the operation title.
    auto setTitle(text::String title) noexcept -> StreamErrorContext &;
    /// Get the operation description.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _description; }
    /// Set the operation description.
    auto setDescription(text::String description) noexcept -> StreamErrorContext &;
    /// Get explicit help or category-derived help when available.
    [[nodiscard]] auto help() const noexcept -> text::String;
    /// Set explicit help, overriding category-derived help.
    auto setHelp(text::String help) noexcept -> StreamErrorContext &;
    /// Get the stream path, or an empty view if none was provided.
    [[nodiscard]] auto path() const noexcept -> const text::String & { return _path; }
    /// Set the stream path.
    auto setPath(text::String path) noexcept -> StreamErrorContext &;
    /// Get the immutable native failure context, if available.
    [[nodiscard]] auto platformContext() const noexcept -> const system::PlatformErrorContextConstPtr & {
        return _platformContext;
    }
    /// Set the immutable native failure context.
    auto setPlatformContext(system::PlatformErrorContextConstPtr platformContext) noexcept -> StreamErrorContext &;

private:
    /// Get help text associated with the captured native error category.
    [[nodiscard]] auto categoryHelp() const noexcept -> text::String;

private:
    text::String _title;                                   ///< Short operation title.
    text::String _description;                             ///< Detailed operation description.
    text::String _help;                                    ///< Explicit recovery guidance.
    text::String _path;                                    ///< Stream path, when available.
    system::PlatformErrorContextConstPtr _platformContext; ///< Embedded native failure details.
};

}
