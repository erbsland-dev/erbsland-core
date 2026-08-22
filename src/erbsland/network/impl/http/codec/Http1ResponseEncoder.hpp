// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Http1Encoder.hpp"

namespace erbsland::network::impl {

/// Bounded incremental HTTP/1.x response encoder.
/// @tested{Http1CodecTest}
class Http1ResponseEncoder final {
public:
    /// Begin encoding a response.
    explicit Http1ResponseEncoder(HttpResponseHead head, HttpMethod requestMethod = {}, Http1CodecLimits limits = {}) :
        _encoder{std::move(head), std::move(requestMethod), limits} {}

public:
    /// Atomically accept one bounded semantic body block.
    [[nodiscard]] auto writeBody(mem::ConstByteSpan bytes) -> NetworkSendStatus { return _encoder.writeBody(bytes); }
    /// Finish the response with optional extension trailers.
    [[nodiscard]] auto finish(HttpHeaders trailers = {}) -> NetworkSendStatus {
        return _encoder.finish(std::move(trailers));
    }
    /// Detach one bounded wire-output block.
    [[nodiscard]] auto takeOutput(unit::ByteLength maximumLength = Http1CodecLimits::cMaximumBodyChunkLength)
        -> mem::ByteBlock {
        return _encoder.takeOutput(maximumLength);
    }
    /// Get the selected body framing.
    [[nodiscard]] auto framing() const noexcept -> Http1BodyFraming { return _encoder.framing(); }
    /// Test whether semantic input is complete.
    [[nodiscard]] auto isComplete() const noexcept -> bool { return _encoder.isComplete(); }
    /// Get queued wire-output bytes.
    [[nodiscard]] auto queuedOutputLength() const noexcept -> unit::ByteLength { return _encoder.queuedOutputLength(); }

private:
    Http1Encoder _encoder; ///< Shared implementation.
};

}
