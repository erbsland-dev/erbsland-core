// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/StreamError.hpp"

#include <memory>

namespace erbsland::stream {

class OutputStream;
using OutputStreamPtr = std::shared_ptr<OutputStream>;

/// The common base class for writable streams.
/// Output streams accept complete write operations. A write method either accepts all provided data or throws
/// `err::StreamError`; it does not silently expose partial writes to user code. Closing a stream flushes pending data
/// according to the concrete stream's semantics.
/// @notest{Abstract interface only; concrete stream implementations require behavior tests.}
class OutputStream {
public:
    virtual ~OutputStream() = default;

public:
    /// Test if the stream is open for writing.
    [[nodiscard]] virtual auto isOpen() const noexcept -> bool = 0;
    /// Flush buffered output.
    /// @throws err::StreamError If the backing target reports a flush error.
    virtual void flush() = 0;
    /// Close the stream.
    /// Closing an already closed stream has no effect.
    /// @throws err::StreamError If pending data cannot be flushed or the backing target reports a close error.
    virtual void close() = 0;
};

}
