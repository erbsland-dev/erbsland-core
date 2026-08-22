// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../unit/ByteLength.hpp"

#include <cstdint>

namespace erbsland::network::impl {

/// Direct RFC 8446 network-order integer and opaque-vector writer.
/// Specification: https://www.rfc-editor.org/rfc/rfc8446.html#section-3
/// @tested{TlsWireCodecTest TlsClientProtocolTest}
class TlsWireWriter final {
public:
    /// Create an empty writer.
    TlsWireWriter() = default;

    // defaults
    ~TlsWireWriter() = default;
    TlsWireWriter(const TlsWireWriter &) = default;
    TlsWireWriter(TlsWireWriter &&) noexcept = default;
    auto operator=(const TlsWireWriter &) -> TlsWireWriter & = default;
    auto operator=(TlsWireWriter &&) noexcept -> TlsWireWriter & = default;

public:
    /// Append one unsigned byte.
    void writeU8(uint8_t value);
    /// Append one network-order uint16.
    void writeU16(uint16_t value);
    /// Append one network-order uint24.
    /// @throws err::ParameterError If `value` exceeds 24 bits.
    void writeU24(uint32_t value);
    /// Append exact opaque bytes without a length prefix.
    void writeBytes(mem::ConstByteSpan value);
    /// Append an opaque vector with a uint8 length.
    /// @throws err::ParameterError If the value exceeds 255 bytes.
    void writeVector8(mem::ConstByteSpan value);
    /// Append an opaque vector with a uint16 length.
    /// @throws err::ParameterError If the value exceeds 65535 bytes.
    void writeVector16(mem::ConstByteSpan value);
    /// Append an opaque vector with a uint24 length.
    /// @throws err::ParameterError If the value exceeds 16777215 bytes.
    void writeVector24(mem::ConstByteSpan value);
    /// Finish the writer and transfer its exact bytes.
    /// @return The complete bytes accumulated so far.
    [[nodiscard]] auto finish() -> mem::ByteBlock;

public: // accessors
    /// Get the current encoded length.
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength { return _data.length(); }
    /// Borrow the current encoded bytes.
    [[nodiscard]] auto span() const noexcept -> mem::ConstByteSpan { return _data.span(); }

private:
    mem::ByteBlockEditor _data; ///< Directly encoded output bytes.
};

}
