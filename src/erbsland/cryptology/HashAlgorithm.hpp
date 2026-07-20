// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CryptographicSecurity.hpp"
#include "CryptographicStatus.hpp"
#include "HashRequirements.hpp"
#include "HashThroughput.hpp"

#include "../text/String_fwd.hpp"
#include "../unit/ByteLength.hpp"
#include "../util/impl/ComparisonHelper.hpp"
#include "../util/List_fwd.hpp"

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
    };

public:
    /// Create the default SHA3-256 algorithm.
    constexpr HashAlgorithm() noexcept = default;
    /// Create an algorithm from its raw value.
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
    /// Get the current usability status.
    [[nodiscard]] auto status() const noexcept -> CryptographicStatus;
    /// Get the current coarse security level.
    [[nodiscard]] auto security() const noexcept -> CryptographicSecurity;
    /// Get the relative throughput of the bundled implementation.
    [[nodiscard]] auto throughput() const noexcept -> HashThroughput;
    /// Test if this algorithm is acceptable with at least standard security.
    [[nodiscard]] auto isSafe() const noexcept -> bool;

public: // requirements
    /// Test if this algorithm satisfies a set of requirements.
    [[nodiscard]] auto matches(const HashRequirements &requirements) const noexcept -> bool;
    /// Get all algorithms that satisfy a set of requirements.
    [[nodiscard]] static auto matching(const HashRequirements &requirements) -> util::List<HashAlgorithm>;
    /// Get the recommended algorithm for a set of requirements.
    /// Selection prefers throughput, then security, then stable declaration order.
    [[nodiscard]] static auto recommended(const HashRequirements &requirements = {}) noexcept
        -> std::optional<HashAlgorithm>;

public: // conversion
    /// Convert the algorithm to its stable lowercase identifier.
    [[nodiscard]] auto toString() const -> text::String;
    /// Parse an exact lowercase algorithm identifier.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<HashAlgorithm>;
    /// Parse an exact lowercase algorithm identifier.
    /// @throws err::ParseError If `text` is not a supported algorithm identifier.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> HashAlgorithm;

public: // enumeration
    /// Get all supported hash algorithms in stable preference order.
    [[nodiscard]] static auto all() noexcept -> std::span<const HashAlgorithm>;
    /// Get all algorithms currently acceptable with at least standard security.
    [[nodiscard]] static auto allSafe() noexcept -> std::span<const HashAlgorithm>;

private:
    Value _value{Sha3_256}; ///< The raw algorithm value.
};

}
