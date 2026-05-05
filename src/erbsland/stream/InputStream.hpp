// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/StreamError.hpp"

#include <memory>

namespace erbsland::stream {

class InputStream;
using InputStreamPtr = std::shared_ptr<InputStream>;

/// The common base class for readable streams.
/// Input streams are stateful objects. Methods that read data advance the stream position when the concrete stream
/// has a position. Reading at end is not an error; read methods report end-of-stream directly instead of requiring a
/// separate end-state check. Operations may throw `err::StreamError` when the stream is closed or the backing source
/// fails.
/// @notest{Abstract interface only; concrete stream implementations require behavior tests.}
class InputStream {
public:
    virtual ~InputStream() = default;

public:
    /// Test if the stream is open for reading.
    [[nodiscard]] virtual auto isOpen() const noexcept -> bool = 0;
    /// Close the stream.
    /// Closing an already closed stream has no effect.
    /// @throws err::StreamError If the backing source reports a close error.
    virtual void close() = 0;
};

}
