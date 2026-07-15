// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BufferedByteInputStreamData_fwd.hpp"
#include "NativeByteStream.hpp"

#include "../ByteInputStream.hpp"

#include <memory>

namespace erbsland::stream::impl {

/// Thread-safe bounded input stream backed by the shared native I/O service.
/// @tested{BufferedStreamTest}
class BufferedByteInputStream final : public ByteInputStream {
public:
    /// Create a buffered input stream around a synchronous native stream.
    /// @param nativeStream The native stream used for source reads.
    /// @param settings The timeout and buffer settings.
    explicit BufferedByteInputStream(NativeByteStreamPtr nativeStream, InputStreamSettings settings = {});
    /// Abort pending work without waiting for native I/O.
    ~BufferedByteInputStream() override;

    BufferedByteInputStream(const BufferedByteInputStream &) = delete;
    BufferedByteInputStream(BufferedByteInputStream &&) = delete;
    auto operator=(const BufferedByteInputStream &) -> BufferedByteInputStream & = delete;
    auto operator=(BufferedByteInputStream &&) -> BufferedByteInputStream & = delete;

public:
    [[nodiscard]] auto inputSettings() const noexcept -> const InputStreamSettings & override;
    [[nodiscard]] auto state() const noexcept -> StreamState override;
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> StreamWaitStatus override;
    auto close() -> StreamCloseStatus override;
    void abort() noexcept override;
    [[nodiscard]] auto createErrorContext() const noexcept -> StreamErrorContext override;

protected: // implement ByteInputStream
    [[nodiscard]] auto readFromSource(std::span<mem::Byte> destination, ReadDeadline deadline)
        -> StreamReadResult<unit::ByteLength> override;
    [[nodiscard]] auto sourceSupportsPositioning() const noexcept -> bool override;
    [[nodiscard]] auto sourcePosition() const -> unit::ByteIndex override;
    auto setSourcePosition(unit::ByteIndex position) -> StreamPositionStatus override;
    auto moveSourcePosition(StreamPositionOrigin origin, unit::ByteOffset offset) -> StreamPositionStatus override;

private:
    [[nodiscard]] auto beginPositioning(std::unique_lock<std::mutex> &lock, ReadDeadline deadline) -> bool;
    void completePositioning(unit::ByteIndex position);
    void cancelPositioning() noexcept;

private:
    BufferedByteInputStreamDataPtr _data;
};

}
