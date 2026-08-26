// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringReader_fwd.hpp"

#include "../U8String.hpp"
#include "../U8StringEditor.hpp"

#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/CpIndex.hpp"
#include "../../impl/StringReaderBase.hpp"

#include <optional>

namespace erbsland::text::impl {

/// A string reader backend for UTF-8 strings.
/// @tested{StringReaderTest}
class U8StringReader final : public StringReaderBase {
public:
    /// Create a reader that owns `text`.
    explicit U8StringReader(U8String text) noexcept;

    // defaults
    U8StringReader(const U8StringReader &) = default;
    U8StringReader(U8StringReader &&) = default;
    auto operator=(const U8StringReader &) -> U8StringReader & = default;
    auto operator=(U8StringReader &&) -> U8StringReader & = default;
    ~U8StringReader() override = default;

public:
    [[nodiscard]] auto clone() const -> U8StringReader * override;
    [[nodiscard]] auto position() const noexcept -> unit::CpIndex override;
    [[nodiscard]] auto save() const noexcept -> StringCharReaderState override;
    void restore(StringCharReaderState state) noexcept override;
    void reset() noexcept override;
    [[nodiscard]] auto read() noexcept -> Char override;
    [[nodiscard]] auto readIf(Char expected) noexcept -> bool override;
    [[nodiscard]] auto readIf(const CharSet &expected) noexcept -> std::optional<Char> override;
    [[nodiscard]] auto peek() const noexcept -> Char override;
    [[nodiscard]] auto isAtEnd() const noexcept -> bool override;
    [[nodiscard]] auto canRead(unit::CpLength count) const noexcept -> bool override;
    auto advance(unit::CpLength count) noexcept -> bool override;
    auto advanceIf(Char expected) noexcept -> bool override;
    auto advanceIf(const CharSet &expected) noexcept -> bool override;
    auto readWhile(const ReadFn &readFn, const CharSet &expected, unit::CpLength maximum) noexcept
        -> util::LoopResult override;
    auto readWhile(const ReadFn &readFn, AsciiCategory expected, unit::CpLength maximum) noexcept
        -> util::LoopResult override;
    auto readUntil(const ReadFn &readFn, const CharSet &stopSet, unit::CpLength maximum) noexcept
        -> util::LoopResult override;
    auto readUntil(const ReadFn &readFn, AsciiCategory stopCategory, unit::CpLength maximum) noexcept
        -> util::LoopResult override;
    auto advanceWhile(const CharSet &expected, unit::CpLength maximum) noexcept -> unit::CpLength override;
    auto advanceWhile(AsciiCategory expected, unit::CpLength maximum) noexcept -> unit::CpLength override;
    auto advanceUntil(const CharSet &stopSet, unit::CpLength maximum) noexcept -> unit::CpLength override;
    auto advanceUntil(AsciiCategory stopCategory, unit::CpLength maximum) noexcept -> unit::CpLength override;
    void startCapture() noexcept override;
    [[nodiscard]] auto takeCapture() noexcept -> AnyString override;
    void clearBuffer() noexcept override;
    [[nodiscard]] auto takeBuffer() -> AnyString override;
    [[nodiscard]] auto bufferView() const noexcept -> AnyString override;
    [[nodiscard]] auto bufferCharacterLength() const noexcept -> unit::CpLength override;
    [[nodiscard]] auto isBufferEmpty() const noexcept -> bool override;
    void setBuffer(const AnyString &text) override;
    void appendToBuffer(Char character) override;
    void appendToBuffer(const AnyString &text) override;
    void appendCaptureToBuffer() override;
    auto readToBuffer() -> Char override;
    [[nodiscard]] auto readToBufferIf(Char expected) -> bool override;
    [[nodiscard]] auto readToBufferIf(const CharSet &expected) -> std::optional<Char> override;
    [[nodiscard]] auto readToBufferWhile(const CharSet &expected, unit::CpLength maximum) -> util::LoopResult override;
    [[nodiscard]] auto readToBufferWhile(AsciiCategory expected, unit::CpLength maximum) -> util::LoopResult override;
    [[nodiscard]] auto readToBufferUntil(const CharSet &stopSet, unit::CpLength maximum) -> util::LoopResult override;
    [[nodiscard]] auto readToBufferUntil(AsciiCategory stopCategory, unit::CpLength maximum)
        -> util::LoopResult override;

private:
    /// Read UTF-8 characters until a matching boundary is reached.
    auto readLoop(const ReadFn &readFn, const CharSet &charSet, unit::CpLength maximum, bool stopOnMatch) noexcept
        -> ReadLoopOutcome;
    /// Read UTF-8 characters until an ASCII-category boundary is reached.
    auto readLoop(const ReadFn &readFn, AsciiCategory category, unit::CpLength maximum, bool stopOnMatch) noexcept
        -> ReadLoopOutcome;
    /// Read UTF-8 characters into the reader buffer until a matching boundary is reached.
    auto readToBufferLoop(const CharSet &charSet, unit::CpLength maximum, bool stopOnMatch) -> util::LoopResult;
    /// Read UTF-8 characters into the reader buffer until an ASCII-category boundary is reached.
    auto readToBufferLoop(AsciiCategory category, unit::CpLength maximum, bool stopOnMatch) -> util::LoopResult;

private:
    U8String _text;                                            ///< The read-only string to read.
    unit::ByteIndex _position{unit::ByteIndex::zero()};        ///< The current byte position.
    unit::CpIndex _cpPosition{unit::CpIndex::zero()};          ///< The current decoded code-point position.
    unit::ByteIndex _captureStart{unit::ByteIndex::noIndex()}; ///< The start position of the capture.
    U8StringEditor _buffer;                                    ///< The reader buffer.
    unit::CpLength _bufferLength{unit::CpLength::zero()};      ///< The decoded code-point length of the buffer.
};

}
