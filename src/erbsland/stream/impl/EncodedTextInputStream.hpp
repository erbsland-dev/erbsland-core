// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EncodedTextInputStream_fwd.hpp"
#include "RetainedTextBuffer_fwd.hpp"

#include "../ByteInputStream.hpp"
#include "../TextInputStream.hpp"

#include "../../text/EncodingMode.hpp"
#include "../../text/StringBomMode.hpp"
#include "../../text/StringDecodeBuffer.hpp"

#include <memory>
#include <mutex>
#include <optional>

namespace erbsland::stream::impl {

/// Text input stream that selects the decoding storage mode from its byte stream settings.
/// @tested{EncodedTextStreamTest}
class EncodedTextInputStream final : public TextInputStream {
private:
    using ReadDeadline = time::TimePoint;

public:
    /// Create an encoded text input stream.
    /// @param byteInputStream The byte stream to read from.
    /// @param encoding The configured text encoding.
    /// @param bomMode How byte order marks are handled.
    /// @param mode How decoding errors are handled.
    /// @throws stream::StreamError If `byteInputStream` is empty.
    explicit EncodedTextInputStream(
        ByteInputStreamPtr byteInputStream,
        text::StringEncoding encoding,
        text::StringBomMode bomMode = text::StringBomMode::Automatic,
        text::EncodingMode mode = text::EncodingMode::Tolerant);

    /// dtor, calling abort on the stream.
    ~EncodedTextInputStream() override;

    // defaults/deletions
    EncodedTextInputStream(const EncodedTextInputStream &) = delete;
    EncodedTextInputStream(EncodedTextInputStream &&) = delete;
    auto operator=(const EncodedTextInputStream &) -> EncodedTextInputStream & = delete;
    auto operator=(EncodedTextInputStream &&) -> EncodedTextInputStream & = delete;

public: // implement InputStream
    [[nodiscard]] auto inputSettings() const noexcept -> const InputStreamSettings & override;
    [[nodiscard]] auto state() const noexcept -> StreamState override;
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> StreamWaitStatus override;
    auto close() -> StreamCloseStatus override;
    void abort() noexcept override;
    [[nodiscard]] auto createErrorContext() const noexcept -> StreamErrorContext override;

public: // implement TextInputStream
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto readChar() -> StreamReadResult<text::Char> override;
    [[nodiscard]] auto read(unit::CpLength maximum) -> StreamReadResult<text::String> override;
    [[nodiscard]] auto readLine(unit::CpLength maximum) -> StreamReadResult<text::String> override;
    [[nodiscard]] auto readAll(unit::CpLength maximum) -> StreamReadResult<text::String> override;

public: // implement StreamPositioning
    [[nodiscard]] auto supportsPositioning() const noexcept -> bool override;
    [[nodiscard]] auto position() const -> unit::ByteIndex override;
    auto setPosition(unit::ByteIndex position) -> StreamPositionStatus override;
    auto movePosition(StreamPositionOrigin origin, unit::ByteOffset offset) -> StreamPositionStatus override;

public:
    using TextInputStream::read;
    using TextInputStream::readAll;
    using TextInputStream::readLine;

    /// Discard all decoded and retained input.
    void discardBufferedInput() noexcept;
    /// Switch the runtime storage policy and discard all pending input.
    void setSensitive(bool sensitive) noexcept;

private:
    /// Get the read deadline derived from the stream settings.
    [[nodiscard]] auto deadlineFromNow() const -> ReadDeadline;
    /// Prepare decoded input up to a maximum character count.
    [[nodiscard]] auto prepareChunk(unit::CpLength maximum, ReadDeadline deadline) -> StreamReadStatus;
    /// Prepare decoded input through one line ending.
    [[nodiscard]] auto prepareLine(unit::CpLength maximum, ReadDeadline deadline) -> StreamReadStatus;
    /// Prepare all remaining decoded input.
    [[nodiscard]] auto prepareAll(unit::CpLength maximum, ReadDeadline deadline) -> StreamReadStatus;
    /// Decode and append one source chunk.
    [[nodiscard]] auto appendDecoded(unit::CpLength maximum, bool stopAtLineEnd) -> bool;
    /// Fill the decoder from the byte stream before a deadline.
    [[nodiscard]] auto fillDecodeBuffer(ReadDeadline deadline) -> StreamReadStatus;
    /// Test whether the decoder contains no input.
    [[nodiscard]] auto decoderIsEmpty() const noexcept -> bool;
    /// Test whether the decoder has a complete character.
    [[nodiscard]] auto decoderHasCharacter() const noexcept -> bool;
    /// Finish the decoder after end of byte input.
    void finishDecoder();
    /// Reset the decoder for an optional continuation encoding.
    void resetDecoder(std::optional<text::StringEncoding> continuationEncoding = std::nullopt);
    /// Get the current byte position while holding the stream lock.
    [[nodiscard]] auto positionLocked() const -> unit::ByteIndex;
    /// Test whether decoding can continue from a nonzero byte position.
    [[nodiscard]] auto canContinueAtNonzeroPosition() const noexcept -> bool;
    /// Reset decoding state after setting a byte position.
    void resetAfterPositioning(unit::ByteIndex position, text::StringEncoding effectiveEncoding);
    /// Return a read result from retained decoded text.
    [[nodiscard]] auto takeResult(StreamReadStatus status, unit::CpLength maximum, bool line)
        -> StreamReadResult<text::String>;

private:
    mutable std::mutex _mutex;
    ByteInputStreamPtr _byteInputStream;
    text::StringEncoding _encoding;
    text::StringBomMode _bomMode;
    text::EncodingMode _mode;
    std::optional<text::StringDecodeBuffer> _decoder;
    std::unique_ptr<RetainedTextBuffer> _retainedText;
    bool _byteInputFinished{false};
    unit::ByteIndex _positionBase{};
};

}
