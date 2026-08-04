// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../unit/ByteLength.hpp"

#include <optional>

namespace erbsland::network::impl {

/// Incremental bounded TLS record deframer for an arbitrary TCP byte stream.
/// The maximum accepted fragment is the RFC 8446 Section 5.2 `TLSCiphertext.length` bound of 2^14 + 256 bytes.
/// Specification: https://www.rfc-editor.org/rfc/rfc8446.html#section-5.1
/// @tested{TlsWireCodecTest TlsClientProtocolTest}
class TlsRecordStream final {
public:
    /// Create a record deframer with an aggregate receive-buffer bound.
    /// @param bufferLimit Maximum incomplete TCP bytes retained.
    explicit TlsRecordStream(unit::ByteLength bufferLimit) noexcept : _bufferLimit{bufferLimit} {}

    // defaults
    ~TlsRecordStream() = default;
    TlsRecordStream(const TlsRecordStream &) = default;
    TlsRecordStream(TlsRecordStream &&) noexcept = default;
    auto operator=(const TlsRecordStream &) -> TlsRecordStream & = default;
    auto operator=(TlsRecordStream &&) noexcept -> TlsRecordStream & = default;

public:
    /// Append exact TCP bytes.
    /// @param data The next stream bytes.
    /// @throws TlsProtocolError If the aggregate incomplete input exceeds the configured bound.
    void append(mem::ConstByteSpan data);
    /// Extract the next complete header-plus-fragment record.
    /// @return A complete record, or no value while more TCP bytes are required.
    /// @throws TlsProtocolError If the record length exceeds the RFC bound.
    [[nodiscard]] auto next() -> std::optional<mem::ByteBlock>;
    /// Erase and release buffered transport bytes.
    void clear() noexcept;

public: // accessors
    /// Get the number of retained incomplete TCP bytes.
    [[nodiscard]] auto bufferedLength() const noexcept -> unit::ByteLength { return _buffer.length(); }

private:
    static constexpr std::size_t cHeaderLength{5U};              ///< TLSPlaintext/TLSCiphertext header size.
    static constexpr std::size_t cMaximumFragmentLength{16640U}; ///< RFC 8446 Section 5.2 ciphertext limit.

    unit::ByteLength _bufferLimit;                               ///< Configured aggregate incomplete input limit.
    mem::ByteBlockEditor _buffer;                                ///< Incremental TCP bytes.
};

}
