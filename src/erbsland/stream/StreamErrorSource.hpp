// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StreamErrorContext.hpp"

namespace erbsland::stream {

/// Extension interface for reporting errors from a stream or a stream adapter.
/// Stream implementations override the context factory to add source-specific information, such as a file path.
/// Callers normally use the stream operation that failed; wrappers use this interface to delegate context creation.
/// @tested{StreamErrorSourceTest}
class StreamErrorSource {
public:
    // defaults
    virtual ~StreamErrorSource() = default;

public:
    /// Throw an error with a user-facing title and explanation.
    /// @param title What operation failed.
    /// @param description Why the operation failed.
    [[noreturn]] void throwError(text::String title, text::String description) const;
    /// Create a diagnostic context for errors reported by this source.
    /// Implementations can add source-specific data or delegate to a wrapped source.
    [[nodiscard]] virtual auto createErrorContext() const noexcept -> StreamErrorContext;
};

}
