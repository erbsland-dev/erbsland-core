// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CryptographicDataBlock.hpp"

#include "../../text/String.hpp"

#include <utility>

namespace erbsland::cryptology {

/// An authentication tag produced or consumed by an AEAD construction.
/// @seedoc{/reference/cryptology/cryptographic_operations}
/// @tested{CryptographicDataBlockTest SymmetricEncryptionFrontendTest}
class SymmetricTag final : public CryptographicDataBlock {
public:
    /// Create an empty tag placeholder.
    SymmetricTag() = default;
    /// Create a tag sharing an owning byte block.
    /// @param data The tag bytes whose shared allocation is marked as sensitive.
    explicit SymmetricTag(mem::ByteBlock data) noexcept : CryptographicDataBlock{std::move(data)} {}
    /// Create a tag by copying borrowed bytes.
    /// @param data The tag bytes to copy into sensitive storage.
    explicit SymmetricTag(mem::ConstByteSpan data) : CryptographicDataBlock{data} {}

    // defaults
    ~SymmetricTag() = default;
    SymmetricTag(const SymmetricTag &) = default;
    SymmetricTag(SymmetricTag &&) noexcept = default;
    auto operator=(const SymmetricTag &) -> SymmetricTag & = default;
    auto operator=(SymmetricTag &&) noexcept -> SymmetricTag & = default;

public: // accessors
    /// Access the owning tag bytes.
    [[nodiscard]] auto data() const noexcept -> const mem::ByteBlock & { return dataBlock(); }
    /// Access a borrowed view of the tag bytes.
    [[nodiscard]] auto span() const noexcept -> mem::ConstByteSpan { return dataSpan(); }

public: // conversion
    /// Convert the tag to compact lowercase hexadecimal text for display or diagnostics.
    [[nodiscard]] auto toString() const -> text::String { return dataToString(); }
};

}
