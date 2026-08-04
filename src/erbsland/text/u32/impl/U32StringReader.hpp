// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32StringReader_fwd.hpp"

#include "../U32String.hpp"
#include "../U32StringEditor.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../impl/StringReaderBase.hpp"

#include <optional>

namespace erbsland::text::impl {

/// A string reader backend for UTF-32 strings.
/// @tested{StringReaderTest}
class U32StringReader final : public StringReaderBase {
public:
    /// Create a reader that owns `text`.
    explicit U32StringReader(U32String text) noexcept;

    // defaults
    U32StringReader(const U32StringReader &) = default;
    U32StringReader(U32StringReader &&) = default;
    auto operator=(const U32StringReader &) -> U32StringReader & = default;
    auto operator=(U32StringReader &&) -> U32StringReader & = default;
    ~U32StringReader() override = default;

public:
    [[nodiscard]] auto clone() const -> U32StringReader * override;
    [[nodiscard]] auto position() const noexcept -> unit::CpIndex override;
    [[nodiscard]] auto save() const noexcept -> StringCharReaderState override;
    auto restore(StringCharReaderState state) noexcept -> bool override;
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
    auto readUntil(const ReadFn &readFn, const CharSet &stopSet, unit::CpLength maximum) noexcept
        -> util::LoopResult override;
    auto advanceWhile(const CharSet &expected, unit::CpLength maximum) noexcept -> unit::CpLength override;
    auto advanceUntil(const CharSet &stopSet, unit::CpLength maximum) noexcept -> unit::CpLength override;
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
    [[nodiscard]] auto readToBufferUntil(const CharSet &stopSet, unit::CpLength maximum) -> util::LoopResult override;

private:
    /// Read UTF-32 characters until a matching boundary is reached.
    auto readLoop(const ReadFn &readFn, const CharSet &charSet, unit::CpLength maximum, bool stopOnMatch) noexcept
        -> ReadLoopOutcome;
    /// Read UTF-32 characters into the reader buffer until a matching boundary is reached.
    auto readToBufferLoop(const CharSet &charSet, unit::CpLength maximum, bool stopOnMatch) -> util::LoopResult;

private:
    U32String _text;                                       ///< The read-only string to read.
    unit::CpIndex _position{unit::CpIndex::zero()};        ///< The current UTF-32 data position.
    unit::CpIndex _cpPosition{unit::CpIndex::zero()};      ///< The current decoded code-point position.
    unit::CpIndex _captureStart{unit::CpIndex::noIndex()}; ///< The start position of the capture.
    U32StringEditor _buffer;                               ///< The reader buffer.
};

}
