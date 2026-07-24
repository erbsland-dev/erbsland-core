// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringReader_fwd.hpp"

#include "../U16String.hpp"
#include "../U16StringEditor.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../../unit/U16DataIndex.hpp"
#include "../../impl/StringReaderBase.hpp"

#include <optional>

namespace erbsland::text::impl {

/// A string reader backend for UTF-16 strings.
/// @tested{StringReaderTest}
class U16StringReader final : public StringReaderBase {
public:
    explicit U16StringReader(U16String text) noexcept;
    U16StringReader(const U16StringReader &) = default;
    U16StringReader(U16StringReader &&) = default;
    auto operator=(const U16StringReader &) -> U16StringReader & = default;
    auto operator=(U16StringReader &&) -> U16StringReader & = default;
    ~U16StringReader() override = default;

public:
    [[nodiscard]] auto clone() const -> U16StringReader * override;
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
    [[nodiscard]] auto readToBuffer() -> Char override;
    [[nodiscard]] auto readToBufferIf(Char expected) -> bool override;
    [[nodiscard]] auto readToBufferIf(const CharSet &expected) -> std::optional<Char> override;
    [[nodiscard]] auto readToBufferWhile(const CharSet &expected, unit::CpLength maximum) -> util::LoopResult override;
    [[nodiscard]] auto readToBufferUntil(const CharSet &stopSet, unit::CpLength maximum) -> util::LoopResult override;

private:
    auto readLoop(const ReadFn &readFn, const CharSet &charSet, unit::CpLength maximum, bool stopOnMatch) noexcept
        -> util::LoopResult;
    auto readToBufferLoop(const CharSet &charSet, unit::CpLength maximum, bool stopOnMatch) -> util::LoopResult;

private:
    U16String _text;                                                 ///< The read-only string to read.
    unit::U16DataIndex _position{unit::U16DataIndex::zero()};        ///< The current UTF-16 data position.
    unit::CpIndex _cpPosition{unit::CpIndex::zero()};                ///< The current decoded code-point position.
    unit::U16DataIndex _captureStart{unit::U16DataIndex::noIndex()}; ///< The start position of the capture.
    U16StringEditor _buffer;                                         ///< The reader buffer.
    unit::CpLength _bufferLength{unit::CpLength::zero()};            ///< The decoded code-point length of the buffer.
};

}
