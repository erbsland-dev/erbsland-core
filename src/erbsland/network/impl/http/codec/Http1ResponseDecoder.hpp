// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Http1Decoder.hpp"

namespace erbsland::network::impl {

/// Strict incremental HTTP/1.x response decoder.
/// @tested{Http1CodecTest}
class Http1ResponseDecoder final {
public:
    /// Create a response decoder with optional originating method context.
    explicit Http1ResponseDecoder(HttpMethod requestMethod = {}, Http1CodecLimits limits = {}) :
        _decoder{Http1Decoder::Role::Response, limits} {
        _decoder.setRequestMethod(std::move(requestMethod));
    }

public:
    /// Atomically append bounded wire input.
    [[nodiscard]] auto feed(mem::ConstByteSpan bytes) -> NetworkSendStatus { return _decoder.feed(bytes); }
    /// Signal transport EOF.
    void endOfInput() { _decoder.endOfInput(); }
    /// Poll the next decoded event.
    [[nodiscard]] auto next() -> std::optional<Http1DecodeEvent> { return _decoder.next(); }
    /// Reset after completion while retaining pipelined bytes.
    void reset() { _decoder.reset(); }
    /// Replace response request-method context before decoding.
    void setRequestMethod(HttpMethod method) { _decoder.setRequestMethod(std::move(method)); }
    /// Detach bytes following a protocol switch or successful CONNECT.
    [[nodiscard]] auto takeOpaqueRemainder() -> mem::ByteBlock { return _decoder.takeOpaqueRemainder(); }
    /// Detach bytes retained after the completed response.
    [[nodiscard]] auto takeRetainedInput() -> mem::ByteBlock { return _decoder.takeRetainedInput(); }
    /// Test whether the current response is complete.
    [[nodiscard]] auto isComplete() const noexcept -> bool { return _decoder.isComplete(); }
    /// Get retained unconsumed input bytes.
    [[nodiscard]] auto retainedInputLength() const noexcept -> unit::ByteLength {
        return _decoder.retainedInputLength();
    }

private:
    Http1Decoder _decoder; ///< Shared implementation.
};

}
