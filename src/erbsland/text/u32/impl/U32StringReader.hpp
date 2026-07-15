// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../U32StringView.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../impl/StringReaderBase.hpp"

#include <optional>

namespace erbsland::text::impl {

/// A string reader backend for UTF-32 strings.
/// @tested{StringReaderTest}
class U32StringReader final : public StringReaderBase {
public:
    explicit U32StringReader(U32StringView text) noexcept;
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
    [[nodiscard]] auto readOrThrow() -> Char override;
    [[nodiscard]] auto readIfOrThrow(Char expected) -> bool override;
    [[nodiscard]] auto readIfOrThrow(const CharSet &expected) -> std::optional<Char> override;
    [[nodiscard]] auto peekOrThrow() const -> Char override;
    [[nodiscard]] auto isAtEnd() const noexcept -> bool override;
    [[nodiscard]] auto canRead(unit::CpLength count) const noexcept -> bool override;
    auto advance(unit::CpLength count) noexcept -> bool override;
    auto advanceIf(Char expected) noexcept -> bool override;
    auto advanceIf(const CharSet &expected) noexcept -> bool override;
    auto advanceIfOrThrow(Char expected) -> bool override;
    auto advanceIfOrThrow(const CharSet &expected) -> bool override;
    auto readWhile(const ReadFn &readFn, const CharSet &expected, unit::CpLength maximum) noexcept
        -> util::LoopResult override;
    auto readUntil(const ReadFn &readFn, const CharSet &stopSet, unit::CpLength maximum) noexcept
        -> util::LoopResult override;
    void startCapture() noexcept override;
    [[nodiscard]] auto takeCapture() noexcept -> AnyStringView override;
    void clearBuffer() noexcept override;
    [[nodiscard]] auto takeBuffer() -> AnyString override;
    [[nodiscard]] auto bufferView() const noexcept -> AnyStringView override;
    [[nodiscard]] auto bufferCharacterLength() const noexcept -> unit::CpLength override;
    [[nodiscard]] auto isBufferEmpty() const noexcept -> bool override;
    void setBuffer(const AnyStringView &text) override;
    void appendToBuffer(Char character) override;
    void appendToBuffer(const AnyStringView &text) override;
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
    U32StringView _text;                                   ///< The string view to read.
    unit::CpIndex _position{unit::CpIndex::zero()};        ///< The current UTF-32 data position.
    unit::CpIndex _cpPosition{unit::CpIndex::zero()};      ///< The current decoded code-point position.
    unit::CpIndex _captureStart{unit::CpIndex::noIndex()}; ///< The start position of the capture.
    U32String _buffer;                                     ///< The reader buffer.
};

}
