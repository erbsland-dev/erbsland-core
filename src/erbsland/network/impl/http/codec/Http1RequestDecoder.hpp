// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Http1Decoder.hpp"

namespace erbsland::network::impl {

/// Strict incremental HTTP/1.x request decoder.
/// @tested{Http1CodecTest}
class Http1RequestDecoder final {
public:
    /// Create a request decoder.
    explicit Http1RequestDecoder(Http1CodecLimits limits = {}) : _decoder{Http1Decoder::Role::Request, limits} {}

public:
    /// Atomically append bounded wire input.
    [[nodiscard]] auto feed(mem::ConstByteSpan bytes) -> NetworkSendStatus { return _decoder.feed(bytes); }
    /// Signal transport EOF.
    void endOfInput() { _decoder.endOfInput(); }
    /// Poll the next decoded event.
    [[nodiscard]] auto next() -> std::optional<Http1DecodeEvent> { return _decoder.next(); }
    /// Reset after completion while retaining pipelined bytes.
    void reset() { _decoder.reset(); }
    /// Detach bytes retained after the completed request.
    [[nodiscard]] auto takeRetainedInput() -> mem::ByteBlock { return _decoder.takeRetainedInput(); }
    /// Test whether the current request is complete.
    [[nodiscard]] auto isComplete() const noexcept -> bool { return _decoder.isComplete(); }
    /// Get retained unconsumed input bytes.
    [[nodiscard]] auto retainedInputLength() const noexcept -> unit::ByteLength {
        return _decoder.retainedInputLength();
    }

private:
    Http1Decoder _decoder; ///< Shared implementation.
};

}
