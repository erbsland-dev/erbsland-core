// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringReaderBackendKind.hpp"
#include "StringReaderBase_fwd.hpp"

#include "../AnyString_fwd.hpp"
#include "../AnyStringEditor_fwd.hpp"
#include "../Char.hpp"
#include "../CharSet.hpp"
#include "../StringCharReaderState.hpp"

#include "../../mem/SharedVirtualData.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/CpLength.hpp"
#include "../../util/LoopResult.hpp"
#include "../../util/LoopStatus.hpp"

#include <cstddef>
#include <optional>
#include <utility>

namespace erbsland::text::impl {

/// Abstract base class for string reader backends.
/// @tested{StringReaderTest}
class StringReaderBase : public mem::SharedVirtualData {
public:
    using ReadFn = std::function<util::LoopStatus(Char)>;

protected:
    /// The status and consumed-character count from a decoded-character loop.
    using ReadLoopOutcome = std::pair<util::LoopResult, unit::CpLength>;

public:
    // defaults
    StringReaderBase() = default;
    StringReaderBase(const StringReaderBase &) = default;
    StringReaderBase(StringReaderBase &&) = default;
    auto operator=(const StringReaderBase &) -> StringReaderBase & = default;
    auto operator=(StringReaderBase &&) -> StringReaderBase & = default;
    ~StringReaderBase() override = default;

public:
    /// Create an unreferenced polymorphic copy.
    [[nodiscard]] auto clone() const -> StringReaderBase * override = 0;
    /// Get the current decoded code-point position.
    [[nodiscard]] virtual auto position() const noexcept -> unit::CpIndex = 0;
    /// Save the current reader state.
    [[nodiscard]] virtual auto save() const noexcept -> StringCharReaderState = 0;
    /// Restore a previous reader state.
    virtual auto restore(StringCharReaderState state) noexcept -> bool = 0;
    /// Reset the reader position to the start of the string.
    virtual void reset() noexcept = 0;
    /// Read a tolerant character and advance on success.
    [[nodiscard]] virtual auto read() noexcept -> Char = 0;
    /// Read a tolerant character only if it matches the expected character.
    [[nodiscard]] virtual auto readIf(Char expected) noexcept -> bool = 0;
    /// Read a tolerant character only if it matches the expected character set.
    [[nodiscard]] virtual auto readIf(const CharSet &expected) noexcept -> std::optional<Char> = 0;
    /// Peek a tolerant character.
    [[nodiscard]] virtual auto peek() const noexcept -> Char = 0;
    /// Test if the reader is at the end of data.
    [[nodiscard]] virtual auto isAtEnd() const noexcept -> bool = 0;
    /// Test if at least `count` decoded characters are available.
    [[nodiscard]] virtual auto canRead(unit::CpLength count) const noexcept -> bool = 0;
    /// Advance the reader.
    virtual auto advance(unit::CpLength count) noexcept -> bool = 0;
    /// Advance by one tolerant character only if it matches the expected character.
    virtual auto advanceIf(Char expected) noexcept -> bool = 0;
    /// Advance by one tolerant character only if it matches the expected character set.
    virtual auto advanceIf(const CharSet &expected) noexcept -> bool = 0;

public: // read loops
    /// Read while expected characters are found.
    virtual auto readWhile(const ReadFn &readFn, const CharSet &expected, unit::CpLength maximum) noexcept
        -> util::LoopResult = 0;
    /// Read until stop characters are found.
    virtual auto readUntil(const ReadFn &readFn, const CharSet &stopSet, unit::CpLength maximum) noexcept
        -> util::LoopResult = 0;
    /// Advance while expected characters are found.
    virtual auto advanceWhile(const CharSet &expected, unit::CpLength maximum) noexcept -> unit::CpLength = 0;
    /// Advance until a stop character is found.
    virtual auto advanceUntil(const CharSet &stopSet, unit::CpLength maximum) noexcept -> unit::CpLength = 0;

public: // capture
    /// Set the capture start position.
    virtual void startCapture() noexcept = 0;
    /// Take the current capture and set the new capture start point.
    [[nodiscard]] virtual auto takeCapture() noexcept -> AnyString = 0;

public: // buffer
    /// Clear the buffer.
    virtual void clearBuffer() noexcept = 0;
    /// Move out the buffer and reset it.
    [[nodiscard]] virtual auto takeBuffer() -> AnyString = 0;
    /// Create a view to the current buffer content.
    [[nodiscard]] virtual auto bufferView() const noexcept -> AnyString = 0;
    /// Get the current decoded code-point length of the buffer.
    [[nodiscard]] virtual auto bufferCharacterLength() const noexcept -> unit::CpLength = 0;
    /// Test if the buffer is empty.
    [[nodiscard]] virtual auto isBufferEmpty() const noexcept -> bool = 0;
    /// Replace the buffer with text.
    virtual void setBuffer(const AnyString &text) = 0;
    /// Append one Unicode code point to the buffer.
    virtual void appendToBuffer(Char character) = 0;
    /// Append text to the buffer.
    virtual void appendToBuffer(const AnyString &text) = 0;
    /// Take the current capture and append it to the buffer.
    virtual void appendCaptureToBuffer() = 0;
    /// Read one tolerant character and append it to the buffer.
    virtual auto readToBuffer() -> Char = 0;
    /// Read one tolerant character if it matches and append it to the buffer.
    [[nodiscard]] virtual auto readToBufferIf(Char expected) -> bool = 0;
    /// Read one tolerant character if it matches and append it to the buffer.
    [[nodiscard]] virtual auto readToBufferIf(const CharSet &expected) -> std::optional<Char> = 0;
    /// Read while expected characters are found and append them to the buffer.
    [[nodiscard]] virtual auto readToBufferWhile(const CharSet &expected, unit::CpLength maximum)
        -> util::LoopResult = 0;
    /// Read until stop characters are found and append read characters to the buffer.
    [[nodiscard]] virtual auto readToBufferUntil(const CharSet &stopSet, unit::CpLength maximum)
        -> util::LoopResult = 0;

protected:
    /// Create a reader state.
    [[nodiscard]] static constexpr auto makeState(
        StringReaderBackendKind kind,
        mem::StorageIdentifier storageId,
        std::size_t rawPosition,
        unit::CpIndex cpPosition) noexcept -> StringCharReaderState {
        return StringCharReaderState{kind, storageId, rawPosition, cpPosition};
    }
    /// Get the backend kind from a saved state.
    [[nodiscard]] static constexpr auto stateKind(StringCharReaderState state) noexcept -> StringReaderBackendKind {
        return state.kind();
    }
    /// Get the storage identity from a saved state.
    [[nodiscard]] static constexpr auto storageId(StringCharReaderState state) noexcept -> mem::StorageIdentifier {
        return state.storageId();
    }
    /// Get the raw backend position from a saved state.
    [[nodiscard]] static constexpr auto rawPosition(StringCharReaderState state) noexcept -> std::size_t {
        return state.rawPosition();
    }
    /// Get the decoded code-point position from a saved state.
    [[nodiscard]] static constexpr auto cpPosition(StringCharReaderState state) noexcept -> unit::CpIndex {
        return state.cpPosition();
    }
    /// Get the identity of a read-only string.
    template <typename View>
    [[nodiscard]] static auto viewStorageId(const View &view) noexcept -> mem::StorageIdentifier {
        return view.storageId();
    }
};

}
