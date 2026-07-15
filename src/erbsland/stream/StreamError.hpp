// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StreamErrorContext.hpp"

#include "../err/RuntimeError.hpp"

#include <exception>
#include <utility>

namespace erbsland::stream {

/// An error related to a stream operation.
/// @tested{DiagnosticTest}
class StreamError final : public err::RuntimeError {
public:
    /// Create a stream error with complete diagnostic context.
    explicit StreamError(StreamErrorContext context, std::exception_ptr cause = {}) noexcept;

    // defaults
    ~StreamError() override = default;

public: // overrides
    [[nodiscard]] auto diagnostic() const -> err::DiagnosticConstPtr override;

public: // accessors
    /// Get the operation title.
    [[nodiscard]] auto title() const noexcept -> const text::StringView & { return _context.title(); }
    /// Get the operation description.
    [[nodiscard]] auto description() const noexcept -> const text::StringView & { return _context.description(); }
    /// Get explicit or category-derived help.
    [[nodiscard]] auto help() const noexcept -> text::StringView { return _context.help(); }
    /// Get the stream path, or an empty view.
    [[nodiscard]] auto path() const noexcept -> const text::StringView & { return _context.path(); }
    /// Get the immutable native failure context, if available.
    [[nodiscard]] auto platformContext() const noexcept -> const system::PlatformErrorContextConstPtr & {
        return _context.platformContext();
    }
    /// Get the complete stream error context.
    [[nodiscard]] auto context() const noexcept -> const StreamErrorContext & { return _context; }

private:
    StreamErrorContext _context; ///< Complete diagnostic context for this error.
};

}
