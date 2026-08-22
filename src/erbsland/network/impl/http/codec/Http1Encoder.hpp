// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Http1BodyFraming.hpp"
#include "Http1CodecLimits.hpp"
#include "Http1TransferCoding.hpp"

#include "../../../../mem/RingBuffer.hpp"
#include "../../../http/HttpRequestHead.hpp"
#include "../../../http/HttpResponseHead.hpp"
#include "../../../source/NetworkSendStatus.hpp"

#include <optional>

namespace erbsland::network::impl {

/// Shared bounded HTTP/1.x encoder implementation.
/// @tested{Http1CodecTest}
class Http1Encoder final {
public:
    /// Begin encoding a validated request head.
    explicit Http1Encoder(HttpRequestHead head, Http1CodecLimits limits = {});
    /// Begin encoding a validated response head with request-method context.
    explicit Http1Encoder(HttpResponseHead head, HttpMethod requestMethod = {}, Http1CodecLimits limits = {});

public: // body/output
    /// Atomically accept one bounded body fragment.
    [[nodiscard]] auto writeBody(mem::ConstByteSpan bytes) -> NetworkSendStatus;
    /// Finish the message with optional extension trailers.
    [[nodiscard]] auto finish(HttpHeaders trailers = {}) -> NetworkSendStatus;
    /// Detach a bounded wire-output block.
    [[nodiscard]] auto takeOutput(unit::ByteLength maximumLength = Http1CodecLimits::cMaximumBodyChunkLength)
        -> mem::ByteBlock;

public: // status
    /// Get the selected wire framing.
    [[nodiscard]] auto framing() const noexcept -> Http1BodyFraming { return _framing; }
    /// Test whether input for this message is complete.
    [[nodiscard]] auto isComplete() const noexcept -> bool { return _complete; }
    /// Get queued wire-output bytes.
    [[nodiscard]] auto queuedOutputLength() const noexcept -> unit::ByteLength { return _output.length(); }

private: // initialization/framing
    /// Serialize a validated request head.
    void beginRequest(const HttpRequestHead &head);
    /// Serialize a validated response head.
    void beginResponse(const HttpResponseHead &head, const HttpMethod &requestMethod);
    /// Select request body framing from validated fields.
    void selectRequestFraming(const HttpRequestHead &head);
    /// Select response body framing from status, fields, and method context.
    void selectResponseFraming(const HttpResponseHead &head, const HttpMethod &requestMethod);
    /// Parse and validate the framing fields shared by requests and responses.
    void selectFieldFraming(const HttpHeaders &headers);

private: // output helpers
    /// Test whether a complete operation fits into the output queue.
    [[nodiscard]] auto canQueue(unit::ByteLength length) const noexcept -> bool;
    /// Append exact native string bytes.
    void appendString(const text::String &value);
    /// Append an internal ASCII framing literal.
    void appendLiteral(std::string_view value);
    /// Append canonical field lines.
    void appendHeaders(const HttpHeaders &headers);
    /// Append one lowercase hexadecimal chunk prefix.
    void appendChunkPrefix(unit::ByteLength length);
    /// Calculate the complete wire length for one chunk body fragment.
    [[nodiscard]] static auto chunkWireLength(unit::ByteLength length) noexcept -> unit::ByteLength;
    /// Validate a field collection against codec limits.
    void validateFields(const HttpHeaders &headers, HttpHeaderLimits limits) const;
    /// Validate trailers and calculate their serialized length.
    [[nodiscard]] auto trailerWireLength(const HttpHeaders &trailers) const -> unit::ByteLength;
    /// Throw a strong application-side misuse failure.
    [[noreturn]] static void misuse(const text::String &message);

private:
    Http1CodecLimits _limits;                                       ///< Captured resource limits.
    mem::RingBuffer _output;                                        ///< Queued serialized output.
    Http1BodyFraming _framing{Http1BodyFraming::None};              ///< Selected framing.
    Http1TransferCoding _transferCoding{Http1TransferCoding::None}; ///< Parsed transfer coding.
    std::optional<unit::ByteLength> _contentLength;                 ///< Validated fixed length.
    unit::ByteLength _bodyWritten;                                  ///< Accepted semantic body bytes.
    unit::ByteLength _bodyRemaining;                                ///< Remaining fixed-length bytes.
    bool _complete{};                                               ///< No more semantic input accepted.
};

}
