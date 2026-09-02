// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CryptographicDataBlock.hpp"

#include "../../text/String.hpp"

#include <utility>

namespace erbsland::cryptology {

/// An initialization vector for a symmetric encryption construction.
/// @seedoc{/reference/cryptology/cryptographic_operations}
/// @tested{CryptographicDataBlockTest SymmetricEncryptionFrontendTest}
class SymmetricIv final : public CryptographicDataBlock {
public:
    /// Create an empty initialization-vector placeholder.
    SymmetricIv() = default;
    /// Create an initialization vector sharing an owning byte block.
    /// @param data The IV bytes whose shared allocation is marked as sensitive.
    explicit SymmetricIv(mem::ByteBlock data) noexcept : CryptographicDataBlock{std::move(data)} {}
    /// Create an initialization vector by copying borrowed bytes.
    /// @param data The IV bytes to copy into sensitive storage.
    explicit SymmetricIv(mem::ConstByteSpan data) : CryptographicDataBlock{data} {}

    // defaults
    ~SymmetricIv() = default;
    SymmetricIv(const SymmetricIv &) = default;
    SymmetricIv(SymmetricIv &&) noexcept = default;
    auto operator=(const SymmetricIv &) -> SymmetricIv & = default;
    auto operator=(SymmetricIv &&) noexcept -> SymmetricIv & = default;

public: // accessors
    /// Access the owning initialization-vector bytes.
    [[nodiscard]] auto data() const noexcept -> const mem::ByteBlock & { return dataBlock(); }
    /// Access a borrowed view of the initialization-vector bytes.
    [[nodiscard]] auto span() const noexcept -> mem::ConstByteSpan { return dataSpan(); }

public: // conversion
    /// Convert the initialization vector to compact lowercase hexadecimal text for display or diagnostics.
    [[nodiscard]] auto toString() const -> text::String { return dataToString(); }
};

}
