// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../mem/ByteSpan.hpp"

#include <cstddef>
#include <optional>

namespace erbsland::network::impl {

/// Incremental bounded RFC 8446 Handshake-message deframer.
/// Handshake messages may be fragmented or coalesced across records, but the initial client accepts at most one MiB
/// for a single body and one MiB plus its four-byte header in aggregate incomplete storage.
/// Specification: https://www.rfc-editor.org/rfc/rfc8446.html#section-4
/// @tested{TlsWireCodecTest TlsClientProtocolTest}
class TlsHandshakeStream final {
public:
    /// Create an empty handshake stream.
    TlsHandshakeStream() = default;

    // defaults
    ~TlsHandshakeStream() = default;
    TlsHandshakeStream(const TlsHandshakeStream &) = default;
    TlsHandshakeStream(TlsHandshakeStream &&) noexcept = default;
    auto operator=(const TlsHandshakeStream &) -> TlsHandshakeStream & = default;
    auto operator=(TlsHandshakeStream &&) noexcept -> TlsHandshakeStream & = default;

public:
    /// Append authenticated or plaintext handshake content.
    /// @param data The next exact handshake bytes without a record header.
    /// @throws TlsProtocolError If the incomplete aggregate bound is exceeded.
    void append(mem::ConstByteSpan data);
    /// Extract the next complete header-plus-body handshake message.
    /// @return A complete message, or no value while more record content is required.
    /// @throws TlsProtocolError If the body exceeds one MiB.
    [[nodiscard]] auto next() -> std::optional<mem::ByteBlock>;
    /// Erase and release buffered handshake bytes.
    void clear() noexcept;

public: // accessors
    /// Get the number of retained incomplete handshake bytes.
    [[nodiscard]] auto bufferedLength() const noexcept -> unit::ByteLength { return _buffer.length(); }

private:
    static constexpr std::size_t cHeaderLength{4U};                 ///< Handshake header size.
    static constexpr std::size_t cMaximumBodyLength{1024U * 1024U}; ///< Fixed per-message security bound.

    mem::ByteBlockEditor _buffer;                                   ///< Incremental handshake bytes.
};

}
