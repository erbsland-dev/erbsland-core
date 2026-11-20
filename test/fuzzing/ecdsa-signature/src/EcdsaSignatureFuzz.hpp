// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteBlock.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

/// Exercise ECDSA signature verification for one fuzz input.
class EcdsaSignatureFuzz final {
public:
    /// Create a harness for one input.
    /// @param data The bytes supplied by libFuzzer.
    explicit EcdsaSignatureFuzz(const std::span<const uint8_t> data) : _data{data} {}

public: // operations
    /// Process the input once.
    /// @return Zero to continue fuzzing.
    [[nodiscard]] auto run() const -> int;

private:
    /// Read a two-byte length from the fuzz input.
    [[nodiscard]] auto lengthAt(const std::size_t offset) const -> std::size_t;

    /// Copy a field from the fuzz input.
    [[nodiscard]] auto blockAt(const std::size_t offset, const std::size_t size) const -> erbsland::mem::ByteBlock;

private:
    static constexpr auto cHeaderSize = std::size_t{8U};             ///< Size of the input length fields.
    static constexpr auto cMaximumPublicKeySize = std::size_t{160U}; ///< Maximum size of the public key field.
    static constexpr auto cMaximumAlgorithmSize = std::size_t{64U}; ///< Maximum size of the algorithm identifier field.
    static constexpr auto cMaximumMessageSize = std::size_t{4096U}; ///< Maximum size of the message field.
    static constexpr auto cMaximumSignatureSize = std::size_t{128U}; ///< Maximum size of the signature field.

    std::span<const uint8_t> _data;                                  ///< Input bytes supplied by libFuzzer.
};
