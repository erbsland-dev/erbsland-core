// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BufferedByteOutputStreamData_fwd.hpp"
#include "NativeByteStream.hpp"

#include "../ByteOutputStream.hpp"

#include "../../text/Char.hpp"
#include "../../text/String.hpp"
#include "../../text/StringBomMode.hpp"
#include "../../text/StringEncoder_fwd.hpp"
#include "../../text/StringEncoding.hpp"
#include "../../time/TimePoint.hpp"

#include <memory>
#include <mutex>

namespace erbsland::stream::impl {

/// Thread-safe atomic output stream backed by fixed front and bounded growing back rings.
/// @tested{BufferedStreamTest}
class BufferedByteOutputStream final : public ByteOutputStream {
public:
    /// Create a buffered output stream around a synchronous native stream.
    /// @param nativeStream The native stream used for target operations.
    /// @param settings The timeout and buffer settings.
    explicit BufferedByteOutputStream(NativeByteStreamPtr nativeStream, OutputStreamSettings settings = {});
    /// Abort pending work without waiting for native I/O.
    ~BufferedByteOutputStream() override;

    BufferedByteOutputStream(const BufferedByteOutputStream &) = delete;
    BufferedByteOutputStream(BufferedByteOutputStream &&) = delete;
    auto operator=(const BufferedByteOutputStream &) -> BufferedByteOutputStream & = delete;
    auto operator=(BufferedByteOutputStream &&) -> BufferedByteOutputStream & = delete;

public:
    [[nodiscard]] auto outputSettings() const noexcept -> const OutputStreamSettings & override;
    [[nodiscard]] auto state() const noexcept -> StreamState override;
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> StreamWaitStatus override;
    auto flush() -> StreamWriteStatus override;
    auto close() -> StreamCloseStatus override;
    void abort() noexcept override;
    [[nodiscard]] auto createErrorContext() const noexcept -> StreamErrorContext override;
    auto write(mem::ConstByteSpan bytes) -> StreamWriteStatus override;

public: // implement StreamPositioning
    [[nodiscard]] auto supportsPositioning() const noexcept -> bool override;
    [[nodiscard]] auto position() const -> unit::ByteIndex override;
    auto setPosition(unit::ByteIndex position) -> StreamPositionStatus override;
    auto movePosition(StreamPositionOrigin origin, unit::ByteOffset offset) -> StreamPositionStatus override;

public:
    /// Atomically encode text directly into the bounded back ring.
    /// @param text The text to encode and enqueue.
    /// @param encoding The target byte encoding.
    /// @param bomMode How to write a byte-order mark.
    /// @return `Success` if all encoded bytes were accepted, or `Timeout` if nothing was accepted.
    auto writeEncodedText(const text::String &text, text::StringEncoding encoding, text::StringBomMode bomMode)
        -> StreamWriteStatus;
    /// Atomically encode one character directly into the bounded back ring.
    /// @param character The character to encode and enqueue.
    /// @param encoding The target byte encoding.
    /// @param bomMode How to write a byte-order mark.
    /// @return `Success` if all encoded bytes were accepted, or `Timeout` if nothing was accepted.
    auto writeEncodedCharacter(text::Char character, text::StringEncoding encoding, text::StringBomMode bomMode)
        -> StreamWriteStatus;

public:
    using ByteOutputStream::write;

private:
    template <typename T>
    auto writeEncoded(const text::StringEncoder<T> &encoder, text::StringEncoding encoding, text::StringBomMode bomMode)
        -> StreamWriteStatus;
    [[nodiscard]] auto beginPositioning(std::unique_lock<std::mutex> &lock, time::TimePoint deadline) -> bool;
    void completePositioning(unit::ByteIndex position);
    void cancelPositioning() noexcept;

private:
    BufferedByteOutputStreamDataPtr _data;
};

}
#include "../../mem/ByteSpan.hpp"
