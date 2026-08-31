// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BufferedByteOutputStream_fwd.hpp"

#include "../ByteOutputStream.hpp"
#include "../TextOutputStream.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../text/StringBomMode.hpp"

#include <mutex>

namespace erbsland::stream::impl {

/// Text output stream that encodes text into a byte output stream.
/// @tested{EncodedTextStreamTest}
class EncodedTextOutputStream final : public TextOutputStream {
public:
    /// Create an encoded text output stream.
    /// @param byteOutputStream The byte stream to write to.
    /// @param encoding The configured text encoding.
    /// @param bomMode How byte order marks are written.
    /// @param initialBomAlreadyHandled Whether an existing target already handled its initial byte-order mark.
    /// @throws stream::StreamError If `byteOutputStream` is empty.
    explicit EncodedTextOutputStream(
        ByteOutputStreamPtr byteOutputStream,
        text::StringEncoding encoding,
        text::StringBomMode bomMode = text::StringBomMode::Automatic,
        bool initialBomAlreadyHandled = false);

    ~EncodedTextOutputStream() override { abort(); }

    // defaults/deletions
    EncodedTextOutputStream(const EncodedTextOutputStream &) = delete;
    EncodedTextOutputStream(EncodedTextOutputStream &&) = delete;
    auto operator=(const EncodedTextOutputStream &) -> EncodedTextOutputStream & = delete;
    auto operator=(EncodedTextOutputStream &&) -> EncodedTextOutputStream & = delete;

public: // implements TextOutputStream
    [[nodiscard]] auto fileIdentity() const noexcept -> system::FileIdentity override;
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto outputSettings() const noexcept -> const OutputStreamSettings & override;
    [[nodiscard]] auto state() const noexcept -> StreamState override;
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> StreamWaitStatus override;
    auto flush() -> StreamWriteStatus override;
    auto close() -> StreamCloseStatus override;
    void abort() noexcept override;
    [[nodiscard]] auto createErrorContext() const noexcept -> StreamErrorContext override;
    auto write(text::Char character) -> StreamWriteStatus override;
    auto write(const text::String &text) -> StreamWriteStatus override;
    auto writeLine() -> StreamWriteStatus override;
    auto writeLine(const text::String &text) -> StreamWriteStatus override;

public: // implements StreamPositioning
    [[nodiscard]] auto supportsPositioning() const noexcept -> bool override;
    [[nodiscard]] auto position() const -> unit::ByteIndex override;
    auto setPosition(unit::ByteIndex position) -> StreamPositionStatus override;
    auto movePosition(StreamPositionOrigin origin, unit::ByteOffset offset) -> StreamPositionStatus override;

public:
    using TextOutputStream::write;
    using TextOutputStream::writeLine;

private:
    /// Select the BOM mode for the next atomic write.
    /// @return The configured mode, or `Reject` after the initial BOM was handled.
    [[nodiscard]] auto bomModeForNextWrite() const noexcept -> text::StringBomMode;
    /// Encode one normalized character for an unbuffered byte stream.
    /// @param character The character to encode.
    /// @param bomMode The byte-order-mark mode for this write.
    /// @return The complete encoded byte block.
    [[nodiscard]] auto encodeCharacter(text::Char character, text::StringBomMode bomMode) const -> mem::ByteBlock;
    /// Write text while the stream mutex is held.
    /// @param text The text to encode and pass atomically to the byte stream.
    /// @return The status returned by the underlying byte stream.
    auto writeLocked(const text::String &text) -> StreamWriteStatus;

private:
    mutable std::mutex _mutex;                                  ///< Protects byte-order-mark and positioning state.
    ByteOutputStreamPtr _byteOutputStream;                      ///< Underlying byte stream receiving encoded output.
    BufferedByteOutputStream *_bufferedByteOutputStream{};      ///< Cached optional direct-encoding capability.
    text::StringEncoding _encoding{text::StringEncoding::Utf8}; ///< Requested output encoding.
    text::StringEncoding _effectiveEncoding{text::StringEncoding::Utf8}; ///< Resolved output encoding.
    text::StringBomMode _bomMode{text::StringBomMode::Automatic};        ///< Configured byte-order-mark mode.
    bool _bomWritten{false}; ///< Whether the initial byte-order-mark decision was handled.
};

}
