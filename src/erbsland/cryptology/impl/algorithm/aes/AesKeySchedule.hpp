// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteArray.hpp"
#include "../../../../mem/ByteSpan.hpp"

#include <cstddef>

namespace erbsland::cryptology::impl {

/// Expanded AES-128 or AES-256 round keys.
/// Key expansion follows FIPS 197, Section 5.2, and stores every byte in `mem::ByteArray` storage.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest}
class AesKeySchedule final {
public:
    /// Create and expand an AES-128 or AES-256 key.
    /// @param key The 16-byte or 32-byte key.
    /// @throws err::ParameterError If the key length is unsupported.
    explicit AesKeySchedule(mem::ConstByteSpan key);

    /// Securely erase expanded key material.
    ~AesKeySchedule() noexcept;

    // defaults/deletions
    AesKeySchedule(const AesKeySchedule &) = delete;
    AesKeySchedule(AesKeySchedule &&) = delete;
    auto operator=(const AesKeySchedule &) -> AesKeySchedule & = delete;
    auto operator=(AesKeySchedule &&) -> AesKeySchedule & = delete;

public:
    /// Securely erase all expanded key bytes and invalidate the schedule.
    void secureErase() noexcept;

public: // accessors
    /// Get the number of AES rounds.
    [[nodiscard]] auto roundCount() const noexcept -> std::size_t { return _roundCount; }
    /// Copy one 16-byte round key.
    /// @param round The round number from zero through `roundCount()`.
    /// @return The round key, or a zero block for an invalid round.
    [[nodiscard]] auto roundKey(std::size_t round) const noexcept -> mem::ByteArray<16>;

private:
    /// Expand the original key into the round-key storage as specified by FIPS 197, Section 5.2.
    void expand(mem::ConstByteSpan key) noexcept;

private:
    mem::ByteArray<240> _roundKeys; ///< Maximum AES-256 expanded key storage.
    std::size_t _roundCount{};      ///< Ten rounds for AES-128 or fourteen rounds for AES-256.
    std::size_t _keyWordCount{};    ///< Four words for AES-128 or eight words for AES-256.
};

}
