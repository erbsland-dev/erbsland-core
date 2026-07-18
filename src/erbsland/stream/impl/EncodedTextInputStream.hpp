// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ByteInputStream.hpp"
#include "../TextInputStream.hpp"

#include "../../text/EncodingErrorMode.hpp"
#include "../../text/StringBomMode.hpp"
#include "../../text/StringDecodeBuffer.hpp"

#include <mutex>

namespace erbsland::stream::impl {

/// Text input stream that decodes text from a byte input stream.
/// @tested{EncodedTextStreamTest}
class EncodedTextInputStream final : public TextInputStream {
public:
    /// Create an encoded text input stream.
    /// @param byteInputStream The byte stream to read from.
    /// @param encoding The configured text encoding.
    /// @param bomMode How byte order marks are handled.
    /// @param errorMode How decoding errors are handled.
    /// @throws stream::StreamError If `byteInputStream` is empty.
    explicit EncodedTextInputStream(
        ByteInputStreamPtr byteInputStream,
        text::StringEncoding encoding,
        text::StringBomMode bomMode = text::StringBomMode::Automatic,
        text::EncodingErrorMode errorMode = text::EncodingErrorMode::Replace);

    // defaults
    ~EncodedTextInputStream() override { abort(); }
    EncodedTextInputStream(const EncodedTextInputStream &) = delete;
    EncodedTextInputStream(EncodedTextInputStream &&) = delete;
    auto operator=(const EncodedTextInputStream &) -> EncodedTextInputStream & = delete;
    auto operator=(EncodedTextInputStream &&) -> EncodedTextInputStream & = delete;

public: // implement TextInputStream
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto inputSettings() const noexcept -> const InputStreamSettings & override;
    [[nodiscard]] auto state() const noexcept -> StreamState override;
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> StreamWaitStatus override;
    auto close() -> StreamCloseStatus override;
    void abort() noexcept override;
    [[nodiscard]] auto createErrorContext() const noexcept -> StreamErrorContext override;
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

private:
    enum class AggregateReadKind : uint8_t {
        None,
        Line,
        All,
    };

private:
    using ReadDeadline = time::TimePoint;

private:
    [[nodiscard]] auto deadlineFromNow() const -> ReadDeadline;
    [[nodiscard]] auto fillDecodeBuffer(ReadDeadline deadline) -> StreamReadStatus;
    [[nodiscard]] auto readDecodedText(unit::CpLength maximum, ReadDeadline deadline) -> StreamReadResult<text::String>;
    [[nodiscard]] auto readLineChunk(unit::CpLength maximum, ReadDeadline deadline) -> StreamReadResult<text::String>;
    [[nodiscard]] auto takeReplay(unit::CpLength maximum) -> text::String;
    [[nodiscard]] auto takeReplayLine(unit::CpLength maximum) -> text::String;
    void selectAggregateRead(AggregateReadKind kind, unit::CpLength target);
    void cancelAggregateRead();
    [[nodiscard]] auto takePending() -> text::String;
    [[nodiscard]] auto pendingLineIsComplete() const -> bool;
    [[nodiscard]] auto sourceIsReadyLocked() const noexcept -> bool;
    [[nodiscard]] auto positionLocked() const -> unit::ByteIndex;
    [[nodiscard]] auto canContinueAtNonzeroPosition() const noexcept -> bool;
    void resetAfterPositioning(unit::ByteIndex position, text::StringEncoding effectiveEncoding);

private:
    mutable std::mutex _mutex;
    ByteInputStreamPtr _byteInputStream;
    text::StringDecodeBuffer _decodeBuffer;
    bool _byteInputFinished{false};
    AggregateReadKind _aggregateKind{AggregateReadKind::None};
    unit::CpLength _aggregateTarget{};
    text::StringEditor _pendingText;
    text::String _replayText;
    unit::ByteIndex _positionBase{}; ///< Byte position at the last decoder reset.
};

}
