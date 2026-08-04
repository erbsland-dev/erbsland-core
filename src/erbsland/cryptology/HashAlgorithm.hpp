// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CryptographicSecurity.hpp"
#include "HashThroughput.hpp"

#include "../text/String_fwd.hpp"
#include "../unit/ByteLength.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace erbsland::cryptology {

/// A fixed-output cryptographic hash algorithm supported by the library.
/// Algorithm metadata is library policy and can change between releases. Persisted data and protocols must store
/// `toString()` for the selected algorithm instead of relying on a future recommendation returning the same value.
/// @seedoc{/reference/cryptology/hashing}
/// @tested{HashAlgorithmTest}
class HashAlgorithm final {
public:
    /// The raw hash algorithm value.
    enum Value : uint8_t {
        Sha3_256, ///< SHA3-256 with a 256-bit digest.
        Sha3_384, ///< SHA3-384 with a 384-bit digest.
        Sha3_512, ///< SHA3-512 with a 512-bit digest.
        Sha2_256, ///< SHA-256 from the SHA-2 family.
        Sha2_384, ///< SHA-384 from the SHA-2 family.
        Sha2_512, ///< SHA-512 from the SHA-2 family.
        Sha1,     ///< Legacy SHA-1, disallowed for new cryptographic results.
        Md5,      ///< Legacy MD5, disallowed for new cryptographic results.
    };

public:
    /// Create the default SHA3-256 algorithm.
    constexpr HashAlgorithm() noexcept = default;
    /// Create an algorithm from its raw value.
    /// @param value The raw algorithm value.
    constexpr HashAlgorithm(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~HashAlgorithm() = default;
    HashAlgorithm(const HashAlgorithm &) = default;
    HashAlgorithm(HashAlgorithm &&) = default;
    auto operator=(const HashAlgorithm &) -> HashAlgorithm & = default;
    auto operator=(HashAlgorithm &&) -> HashAlgorithm & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const HashAlgorithm &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const HashAlgorithm &other, value, other._value);

public: // accessors
    /// Get the raw algorithm value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }
    /// Get the length of the generated digest.
    [[nodiscard]] auto digestSize() const noexcept -> unit::ByteLength;
    /// Get the current coarse security level.
    [[nodiscard]] auto security() const noexcept -> CryptographicSecurity;
    /// Get the relative throughput of the bundled implementation.
    [[nodiscard]] auto throughput() const noexcept -> HashThroughput;

public: // conversion
    /// Convert the algorithm to its stable lowercase identifier.
    [[nodiscard]] auto toString() const -> text::String;

public: // factories
    /// Parse an exact lowercase algorithm identifier.
    /// @param text The identifier to parse.
    /// @return The matching algorithm, or no value for unsupported text.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<HashAlgorithm>;
    /// Parse an exact lowercase algorithm identifier.
    /// @param text The identifier to parse.
    /// @return The matching algorithm.
    /// @throws err::ParseError If `text` is not a supported algorithm identifier.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> HashAlgorithm;
    /// Get all supported hash algorithms in stable preference order.
    [[nodiscard]] static auto all() noexcept -> std::span<const HashAlgorithm>;

private:
    Value _value{Sha3_256}; ///< The raw algorithm value.
};

}
