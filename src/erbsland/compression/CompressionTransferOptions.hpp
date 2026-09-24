// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompressionFallbackPolicy.hpp"
#include "CompressionProgress.hpp"

namespace erbsland::compression {

/// I/O and buffering policy for one borrowed-stream transfer.
/// @tested{CompressionStreamingTest}
class CompressionTransferOptions final {
public:
    /// Get the optional exact input prefix length.
    [[nodiscard]] auto inputLength() const noexcept -> const std::optional<unit::ByteLength> & { return _inputLength; }
    /// Set the exact input prefix length.
    auto setInputLength(unit::ByteLength value) -> CompressionTransferOptions & {
        _inputLength = value;
        return *this;
    }
    /// Clear the input prefix constraint.
    auto clearInputLength() -> CompressionTransferOptions & {
        _inputLength.reset();
        return *this;
    }
    /// Get the transfer buffer size.
    [[nodiscard]] auto bufferLength() const noexcept -> unit::ByteLength { return _bufferLength; }
    /// Set the positive finite transfer buffer size.
    auto setBufferLength(unit::ByteLength value) -> CompressionTransferOptions & {
        _bufferLength = value;
        return *this;
    }
    /// Get the limit for each complete fallback value.
    [[nodiscard]] auto maximumBufferedLength() const noexcept -> unit::ByteLength { return _maximumBufferedLength; }
    /// Set the fallback input and output limit.
    auto setMaximumBufferedLength(unit::ByteLength value) -> CompressionTransferOptions & {
        _maximumBufferedLength = value;
        return *this;
    }
    /// Get the fallback policy.
    [[nodiscard]] auto fallbackPolicy() const noexcept -> CompressionFallbackPolicy { return _fallbackPolicy; }
    /// Set the fallback policy.
    auto setFallbackPolicy(CompressionFallbackPolicy value) -> CompressionTransferOptions & {
        _fallbackPolicy = value;
        return *this;
    }
    /// Get the progress observer.
    [[nodiscard]] auto progress() const noexcept -> const CompressionProgressFn & { return _progress; }
    /// Set the progress observer.
    auto setProgress(CompressionProgressFn value) -> CompressionTransferOptions & {
        _progress = std::move(value);
        return *this;
    }

private:
    std::optional<unit::ByteLength> _inputLength;                                        ///< Exact source prefix.
    unit::ByteLength _bufferLength{65536U};                                              ///< I/O chunk size.
    unit::ByteLength _maximumBufferedLength{256U * 1024U * 1024U};                       ///< Fallback cap per value.
    CompressionFallbackPolicy _fallbackPolicy{CompressionFallbackPolicy::AllowBuffered}; ///< Fallback policy.
    CompressionProgressFn _progress;                                                     ///< Synchronous observer.
};
}
