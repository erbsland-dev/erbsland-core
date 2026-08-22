// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../unit/ByteLength.hpp"
#include "../../../http/HttpHeaderLimits.hpp"

namespace erbsland::network::impl {

/// Captured resource limits for an internal HTTP/1.x codec.
/// @tested{Http1CodecTest}
class Http1CodecLimits final {
public:
    /// Default maximum start-line length.
    static constexpr auto cDefaultMaximumStartLineLength = unit::ByteLength{8U * 1024U};
    /// Default maximum chunk-size and extension line length.
    static constexpr auto cDefaultMaximumChunkMetadataLength = unit::ByteLength{8U * 1024U};
    /// Default maximum decoded body length.
    static constexpr auto cDefaultMaximumBodyLength = unit::ByteLength{16U * 1024U * 1024U};
    /// Default maximum retained input or queued output.
    static constexpr auto cDefaultMaximumQueueLength = unit::ByteLength{1024U * 1024U};
    /// Maximum body bytes returned or accepted in one event.
    static constexpr auto cMaximumBodyChunkLength = unit::ByteLength{16U * 1024U};
    /// Initial allocation for retained input and queued output.
    static constexpr auto cInitialQueueLength = unit::ByteLength{8U * 1024U};

public: // queue storage
    /// Get the initial physical ring-buffer capacity for a configured queue limit.
    [[nodiscard]] static constexpr auto initialQueueCapacity(const unit::ByteLength maximum) noexcept
        -> unit::ByteLength {
        if (maximum.isZero()) {
            return unit::ByteLength::one();
        }
        return maximum < cInitialQueueLength ? maximum : cInitialQueueLength;
    }
    /// Get the physical ring-buffer limit for a configured queue limit.
    [[nodiscard]] static constexpr auto queueStorageLimit(const unit::ByteLength maximum) noexcept -> unit::ByteLength {
        return maximum.isZero() ? unit::ByteLength::one() : maximum;
    }

public: // accessors
    /// Get the maximum start-line length.
    [[nodiscard]] constexpr auto maximumStartLineLength() const noexcept -> unit::ByteLength { return _startLine; }
    /// Set the maximum start-line length.
    constexpr auto setMaximumStartLineLength(unit::ByteLength value) noexcept -> Http1CodecLimits & {
        _startLine = value;
        return *this;
    }
    /// Get header limits.
    [[nodiscard]] constexpr auto headerLimits() const noexcept -> HttpHeaderLimits { return _headers; }
    /// Set header limits.
    constexpr auto setHeaderLimits(HttpHeaderLimits value) noexcept -> Http1CodecLimits & {
        _headers = value;
        return *this;
    }
    /// Get trailer limits.
    [[nodiscard]] constexpr auto trailerLimits() const noexcept -> HttpHeaderLimits { return _trailers; }
    /// Set trailer limits.
    constexpr auto setTrailerLimits(HttpHeaderLimits value) noexcept -> Http1CodecLimits & {
        _trailers = value;
        return *this;
    }
    /// Get the maximum chunk metadata length.
    [[nodiscard]] constexpr auto maximumChunkMetadataLength() const noexcept -> unit::ByteLength {
        return _chunkMetadata;
    }
    /// Set the maximum chunk metadata length.
    constexpr auto setMaximumChunkMetadataLength(unit::ByteLength value) noexcept -> Http1CodecLimits & {
        _chunkMetadata = value;
        return *this;
    }
    /// Get the maximum decoded body length.
    [[nodiscard]] constexpr auto maximumBodyLength() const noexcept -> unit::ByteLength { return _body; }
    /// Set the maximum decoded body length.
    constexpr auto setMaximumBodyLength(unit::ByteLength value) noexcept -> Http1CodecLimits & {
        _body = value;
        return *this;
    }
    /// Get the maximum retained input length.
    [[nodiscard]] constexpr auto maximumInputLength() const noexcept -> unit::ByteLength { return _input; }
    /// Set the maximum retained input length.
    constexpr auto setMaximumInputLength(unit::ByteLength value) noexcept -> Http1CodecLimits & {
        _input = value;
        return *this;
    }
    /// Get the maximum queued output length.
    [[nodiscard]] constexpr auto maximumOutputLength() const noexcept -> unit::ByteLength { return _output; }
    /// Set the maximum queued output length.
    constexpr auto setMaximumOutputLength(unit::ByteLength value) noexcept -> Http1CodecLimits & {
        _output = value;
        return *this;
    }

private:
    unit::ByteLength _startLine{cDefaultMaximumStartLineLength};         ///< Start-line bound.
    HttpHeaderLimits _headers;                                           ///< Header bounds.
    HttpHeaderLimits _trailers;                                          ///< Trailer bounds.
    unit::ByteLength _chunkMetadata{cDefaultMaximumChunkMetadataLength}; ///< Chunk metadata bound.
    unit::ByteLength _body{cDefaultMaximumBodyLength};                   ///< Decoded body bound.
    unit::ByteLength _input{cDefaultMaximumQueueLength};                 ///< Retained input bound.
    unit::ByteLength _output{cDefaultMaximumQueueLength};                ///< Queued output bound.
};

}
