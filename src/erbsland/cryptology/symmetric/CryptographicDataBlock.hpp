// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../impl/symmetric/UnsafeSymmetricKeyAccess_fwd.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../text/String_fwd.hpp"
#include "../../unit/ByteLength.hpp"

#include <cstddef>

namespace erbsland::cryptology {

/// Shared storage behavior for strongly typed cryptographic byte blocks.
/// Every non-empty allocation is permanently marked as sensitive and securely erased on final release.
/// @seedoc{/reference/cryptology/cryptographic_operations}
/// @tested{CryptographicDataBlockTest}
class CryptographicDataBlock {
    friend class impl::UnsafeSymmetricKeyAccess;

protected:
    /// Create an empty cryptographic data block.
    CryptographicDataBlock() = default;
    /// Create a cryptographic data block sharing an owning byte block.
    /// @param data The byte block whose shared allocation is marked as sensitive.
    explicit CryptographicDataBlock(mem::ByteBlock data) noexcept;
    /// Create a cryptographic data block by copying a borrowed span.
    /// @param data The bytes to copy into sensitive storage.
    explicit CryptographicDataBlock(mem::ConstByteSpan data);

    // defaults
    ~CryptographicDataBlock() = default;
    CryptographicDataBlock(const CryptographicDataBlock &) = default;
    CryptographicDataBlock(CryptographicDataBlock &&) noexcept = default;
    auto operator=(const CryptographicDataBlock &) -> CryptographicDataBlock & = default;
    auto operator=(CryptographicDataBlock &&) noexcept -> CryptographicDataBlock & = default;

public: // tests
    /// Test if no data is set.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _data.isEmpty(); }

public: // accessors
    /// Get the number of bytes.
    [[nodiscard]] auto byteLength() const noexcept -> unit::ByteLength { return _data.length(); }
    /// Get the number of bits.
    [[nodiscard]] auto bitLength() const noexcept -> std::size_t { return byteLength().toSizeT() * 8U; }

protected:
    /// Access the owning byte block in derived public wrappers.
    [[nodiscard]] auto dataBlock() const noexcept -> const mem::ByteBlock & { return _data; }
    /// Access a borrowed view in derived public wrappers.
    [[nodiscard]] auto dataSpan() const noexcept -> mem::ConstByteSpan { return _data.span(); }
    /// Format the data as compact lowercase hexadecimal text.
    [[nodiscard]] auto dataToString() const -> text::String;

private:
    mem::ByteBlock _data; ///< Shared sensitive byte storage.
};

}
