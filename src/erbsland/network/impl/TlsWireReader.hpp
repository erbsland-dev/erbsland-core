// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../mem/ByteSpan.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::network::impl {

/// Strict bounded reader for RFC 8446 network-order integers and opaque vectors.
/// Every operation checks its complete length before advancing, and `requireEnd()` makes trailing bytes observable.
/// Specification: https://www.rfc-editor.org/rfc/rfc8446.html#section-3
/// @tested{TlsWireCodecTest TlsClientProtocolTest}
class TlsWireReader final {
public:
    /// Read from one complete enclosing protocol value.
    /// @param data The exact enclosing value bytes.
    explicit TlsWireReader(mem::ConstByteSpan data) noexcept : _data{data} {}

    // defaults
    ~TlsWireReader() = default;
    TlsWireReader(const TlsWireReader &) = default;
    TlsWireReader(TlsWireReader &&) noexcept = default;
    auto operator=(const TlsWireReader &) -> TlsWireReader & = default;
    auto operator=(TlsWireReader &&) noexcept -> TlsWireReader & = default;

public:
    /// Read one unsigned byte.
    [[nodiscard]] auto readU8() -> uint8_t;
    /// Read one network-order uint16.
    [[nodiscard]] auto readU16() -> uint16_t;
    /// Read one network-order uint24 into a native 32-bit value.
    [[nodiscard]] auto readU24() -> uint32_t;
    /// Read one network-order uint32.
    [[nodiscard]] auto readU32() -> uint32_t;
    /// Borrow an exact number of bytes.
    /// @param length The required byte count.
    /// @return A view into the original enclosing value.
    [[nodiscard]] auto readBytes(std::size_t length) -> mem::ConstByteSpan;
    /// Read an opaque vector with a uint8 length.
    [[nodiscard]] auto readVector8() -> mem::ConstByteSpan;
    /// Read an opaque vector with a uint16 length.
    [[nodiscard]] auto readVector16() -> mem::ConstByteSpan;
    /// Read an opaque vector with a uint24 length.
    [[nodiscard]] auto readVector24() -> mem::ConstByteSpan;
    /// Require exact exhaustion of the enclosing value.
    void requireEnd() const;

public: // tests/accessors
    /// Test if all enclosing bytes were consumed.
    [[nodiscard]] auto isAtEnd() const noexcept -> bool { return _offset == _data.size(); }
    /// Get the number of unconsumed bytes.
    [[nodiscard]] auto remaining() const noexcept -> std::size_t { return _data.size() - _offset; }

private:
    /// Require an exact number of remaining bytes.
    void require(std::size_t length) const;

private:
    mem::ConstByteSpan _data; ///< Borrowed enclosing protocol value.
    std::size_t _offset{0U};  ///< Next unread byte offset.
};

}
