// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Http1CodecLimits.hpp"
#include "Http1DecodeEvent.hpp"
#include "Http1FailureReason.hpp"
#include "Http1TransferCoding.hpp"

#include "../../../../mem/RingBuffer.hpp"
#include "../../../http/HttpMethod.hpp"
#include "../../../source/NetworkSendStatus.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::network::impl {

/// Shared strict incremental HTTP/1.x decoder implementation.
/// @tested{Http1CodecTest}
class Http1Decoder final {
public:
    /// The message role decoded by this state machine.
    enum class Role : std::uint8_t {
        Request,  ///< Decode a request.
        Response, ///< Decode a response.
    };

public:
    /// Create a decoder.
    explicit Http1Decoder(Role role, Http1CodecLimits limits = {});

public: // input
    /// Atomically append input, applying retained-input back pressure.
    [[nodiscard]] auto feed(mem::ConstByteSpan bytes) -> NetworkSendStatus;
    /// Signal transport EOF explicitly.
    void endOfInput();
    /// Poll the next decoded event.
    [[nodiscard]] auto next() -> std::optional<Http1DecodeEvent>;
    /// Reset a completed decoder while preserving pipelined input.
    void reset();
    /// Set the originating request method used for response body rules.
    void setRequestMethod(HttpMethod method);
    /// Detach all bytes following a switched protocol or successful CONNECT response.
    [[nodiscard]] auto takeOpaqueRemainder() -> mem::ByteBlock;
    /// Detach all retained bytes after a completed message.
    [[nodiscard]] auto takeRetainedInput() -> mem::ByteBlock;

public: // status
    /// Test whether this decoder completed its current message.
    [[nodiscard]] auto isComplete() const noexcept -> bool;
    /// Test whether transport EOF was signalled.
    [[nodiscard]] auto isEndOfInput() const noexcept -> bool { return _endOfInput; }
    /// Get the number of retained input bytes.
    [[nodiscard]] auto retainedInputLength() const noexcept -> unit::ByteLength { return _input.length(); }

private:
    /// Internal incremental parse states.
    enum class State : std::uint8_t {
        StartLine,       ///< Waiting for a request or status line.
        Headers,         ///< Reading the main field section.
        FixedBody,       ///< Reading a fixed-length body.
        ChunkLine,       ///< Reading chunk metadata.
        ChunkData,       ///< Reading decoded chunk data.
        ChunkDataEnd,    ///< Reading CRLF after chunk data.
        Trailers,        ///< Reading a separate trailer section.
        CloseBody,       ///< Reading a response body until EOF.
        CompletePending, ///< Completion has not yet been emitted.
        Complete,        ///< The message is complete.
        OpaquePending,   ///< Opaque completion has not yet been emitted.
        Opaque,          ///< The retained bytes belong to another protocol.
        Failed,          ///< A terminal protocol failure occurred.
    };

private: // polling
    /// Parse and emit a start-line event when possible.
    [[nodiscard]] auto readStartLine() -> std::optional<Http1DecodeEvent>;
    /// Parse and emit a completed header event when possible.
    [[nodiscard]] auto readHeaders() -> std::optional<Http1DecodeEvent>;
    /// Emit one fixed-length body block when possible.
    [[nodiscard]] auto readFixedBody() -> std::optional<Http1DecodeEvent>;
    /// Parse one chunk-size line when possible.
    [[nodiscard]] auto readChunkLine() -> std::optional<Http1DecodeEvent>;
    /// Emit one decoded chunk body block when possible.
    [[nodiscard]] auto readChunkData() -> std::optional<Http1DecodeEvent>;
    /// Consume the CRLF following chunk data when possible.
    [[nodiscard]] auto readChunkDataEnd() -> std::optional<Http1DecodeEvent>;
    /// Parse and emit the separate trailer section when possible.
    [[nodiscard]] auto readTrailers() -> std::optional<Http1DecodeEvent>;
    /// Emit one close-delimited response body block when possible.
    [[nodiscard]] auto readCloseBody() -> std::optional<Http1DecodeEvent>;
    /// Emit the one completion event and enter the terminal state.
    [[nodiscard]] auto emitCompletion() -> Http1DecodeEvent;

private: // parsing
    /// Detach one strictly CRLF-terminated bounded line when available.
    [[nodiscard]] auto takeLine(unit::ByteLength limit, Http1FailureReason limitReason) -> std::optional<text::String>;
    /// Parse a strict request line.
    void parseRequestLine(const text::String &line);
    /// Parse a strict status line.
    void parseStatusLine(const text::String &line);
    /// Parse one header or trailer field line.
    [[nodiscard]] auto parseFieldLine(const text::String &line, bool trailer) -> HttpField;
    /// Construct the bounded field collection under construction.
    [[nodiscard]] auto finishFields(bool trailer) -> HttpHeaders;
    /// Validate fields and select message-body framing.
    void selectFraming(const HttpHeaders &headers);
    /// Select request body framing.
    void selectRequestFraming();
    /// Select response body framing using status and method context.
    void selectResponseFraming();
    /// Add decoded body bytes under the configured aggregate bound.
    void accountBody(unit::ByteLength length);
    /// Remove bytes from the front of retained input.
    void consumeInput(unit::ByteLength length);
    /// Detach bytes from the front of retained input.
    [[nodiscard]] auto takeInput(unit::ByteLength length) -> mem::ByteBlock;
    /// Enter the failed state and throw a categorized error.
    [[noreturn]] void fail(Http1FailureReason reason, const text::String &message);

private:
    Role _role;                                                     ///< Request or response role.
    Http1CodecLimits _limits;                                       ///< Captured resource limits.
    State _state{State::StartLine};                                 ///< Current parse state.
    mem::RingBuffer _input;                                         ///< Retained unconsumed wire input.
    bool _endOfInput{};                                             ///< Explicit transport EOF flag.
    HttpMethod _requestMethod;                                      ///< Response request-method context.
    HttpMethod _method;                                             ///< Parsed request method.
    text::String _target;                                           ///< Parsed request-target.
    HttpVersion _version;                                           ///< Parsed protocol version.
    HttpStatus _status;                                             ///< Parsed response status.
    text::String _reason;                                           ///< Parsed exact reason phrase.
    HttpFieldList _fields;                                          ///< Fields under construction.
    unit::ByteLength _fieldBytes;                                   ///< Canonical aggregate field bytes.
    Http1BodyFraming _framing{Http1BodyFraming::None};              ///< Selected body framing.
    Http1TransferCoding _transferCoding{Http1TransferCoding::None}; ///< Parsed transfer coding.
    std::optional<unit::ByteLength> _contentLength;                 ///< Validated fixed length.
    unit::ByteLength _bodyRemaining;                                ///< Remaining fixed/chunk bytes.
    unit::ByteLength _bodyDecoded;                                  ///< Total decoded body bytes.
};

}
