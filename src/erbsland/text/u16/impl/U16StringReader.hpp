// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringReader_fwd.hpp"

#include "../U16String.hpp"
#include "../U16StringEditor.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../../unit/U16Index.hpp"
#include "../../impl/StringReaderBase.hpp"

#include <optional>

namespace erbsland::text::impl {

/// A string reader backend for UTF-16 strings.
/// @tested{StringCharReaderTest}
class U16StringReader final : public StringReaderBase {
public:
    /// Create a reader that owns `text`.
    explicit U16StringReader(U16String text) noexcept;

    // defaults
    U16StringReader(const U16StringReader &) = default;
    U16StringReader(U16StringReader &&) = default;
    auto operator=(const U16StringReader &) -> U16StringReader & = default;
    auto operator=(U16StringReader &&) -> U16StringReader & = default;
    ~U16StringReader() override = default;

public:
    [[nodiscard]] auto clone() const -> U16StringReader * override;
    [[nodiscard]] auto position() const noexcept -> unit::CpIndex override;
    [[nodiscard]] auto u16Position() const noexcept -> unit::U16Index override;
    [[nodiscard]] auto save() const noexcept -> StringCharReaderState override;
    void restore(StringCharReaderState state) noexcept override;
    void reset() noexcept override;
    [[nodiscard]] auto read() noexcept -> Char override;
    [[nodiscard]] auto readIf(Char expected) noexcept -> bool override;
    [[nodiscard]] auto readIf(const CharSet &expected) noexcept -> std::optional<Char> override;
    [[nodiscard]] auto peek() const noexcept -> Char override;
    [[nodiscard]] auto isAtEnd() const noexcept -> bool override;
    [[nodiscard]] auto canRead(unit::CpCount count) const noexcept -> bool override;
    auto advance(unit::CpCount count) noexcept -> bool override;
    auto advanceIf(Char expected) noexcept -> bool override;
    auto advanceIf(const CharSet &expected) noexcept -> bool override;
    auto readWhile(const ReadFn &readFn, const CharSet &expected, unit::CpCount maximum) noexcept
        -> util::LoopResult override;
    auto readWhile(const ReadFn &readFn, AsciiCategory expected, unit::CpCount maximum) noexcept
        -> util::LoopResult override;
    auto readUntil(const ReadFn &readFn, const CharSet &stopSet, unit::CpCount maximum) noexcept
        -> util::LoopResult override;
    auto readUntil(const ReadFn &readFn, AsciiCategory stopCategory, unit::CpCount maximum) noexcept
        -> util::LoopResult override;
    auto advanceWhile(const CharSet &expected, unit::CpCount maximum) noexcept -> unit::CpCount override;
    auto advanceWhile(AsciiCategory expected, unit::CpCount maximum) noexcept -> unit::CpCount override;
    auto advanceUntil(const CharSet &stopSet, unit::CpCount maximum) noexcept -> unit::CpCount override;
    auto advanceUntil(AsciiCategory stopCategory, unit::CpCount maximum) noexcept -> unit::CpCount override;
    void startCapture() noexcept override;
    [[nodiscard]] auto takeCapture() noexcept -> AnyString override;
    void clearBuffer() noexcept override;
    [[nodiscard]] auto takeBuffer() -> AnyString override;
    [[nodiscard]] auto bufferView() const noexcept -> AnyString override;
    [[nodiscard]] auto bufferCharacterLength() const noexcept -> unit::CpCount override;
    [[nodiscard]] auto isBufferEmpty() const noexcept -> bool override;
    void setBuffer(const AnyString &text) override;
    void appendToBuffer(Char character) override;
    void appendToBuffer(const AnyString &text) override;
    void appendCaptureToBuffer() override;
    auto readToBuffer() -> Char override;
    [[nodiscard]] auto readToBufferIf(Char expected) -> bool override;
    [[nodiscard]] auto readToBufferIf(const CharSet &expected) -> std::optional<Char> override;
    [[nodiscard]] auto readToBufferWhile(const CharSet &expected, unit::CpCount maximum) -> util::LoopResult override;
    [[nodiscard]] auto readToBufferWhile(AsciiCategory expected, unit::CpCount maximum) -> util::LoopResult override;
    [[nodiscard]] auto readToBufferUntil(const CharSet &stopSet, unit::CpCount maximum) -> util::LoopResult override;
    [[nodiscard]] auto readToBufferUntil(AsciiCategory stopCategory, unit::CpCount maximum)
        -> util::LoopResult override;

private:
    /// Read UTF-16 characters until a matching boundary is reached.
    auto readLoop(const ReadFn &readFn, const CharSet &charSet, unit::CpCount maximum, bool stopOnMatch) noexcept
        -> ReadLoopOutcome;
    /// Read UTF-16 characters until an ASCII-category boundary is reached.
    auto readLoop(const ReadFn &readFn, AsciiCategory category, unit::CpCount maximum, bool stopOnMatch) noexcept
        -> ReadLoopOutcome;
    /// Read UTF-16 characters into the reader buffer until a matching boundary is reached.
    auto readToBufferLoop(const CharSet &charSet, unit::CpCount maximum, bool stopOnMatch) -> util::LoopResult;
    /// Read UTF-16 characters into the reader buffer until an ASCII-category boundary is reached.
    auto readToBufferLoop(AsciiCategory category, unit::CpCount maximum, bool stopOnMatch) -> util::LoopResult;

private:
    U16String _text;                                         ///< The read-only string to read.
    unit::U16Index _position{unit::U16Index::zero()};        ///< The current UTF-16 data position.
    unit::CpIndex _cpPosition{unit::CpIndex::zero()};        ///< The current decoded code-point position.
    unit::U16Index _captureStart{unit::U16Index::noIndex()}; ///< The start position of the capture.
    U16StringEditor _buffer;                                 ///< The reader buffer.
    unit::CpCount _bufferLength{unit::CpCount::zero()};      ///< The decoded code-point length of the buffer.
};

}
