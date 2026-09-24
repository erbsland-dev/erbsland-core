// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace erbsland::compression {

/// A byte-compression algorithm supported by the library.
/// Raw algorithm values are stable because they are stored in compression envelopes.
/// @seedoc{/reference/compression/data_compression}
/// @tested{ByteCompressionTest}
class CompressionAlgorithm final {
public:
    /// The raw compression algorithm value.
    enum Value : uint8_t {
        Lz4Block = 1U,  ///< Standard raw LZ4 block compression.
        Deflate = 2U,   ///< RFC 1951 Deflate compression.
        Bzip2 = 3U,     ///< Bzip2 block-sorting compression.
        Lzma = 4U,      ///< LZMA1 compression.
        Zstandard = 5U, ///< Zstandard compression.
    };

public:
    /// Create the default LZ4 block algorithm.
    constexpr CompressionAlgorithm() noexcept = default;
    /// Create an algorithm from its raw value.
    constexpr CompressionAlgorithm(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~CompressionAlgorithm() = default;
    CompressionAlgorithm(const CompressionAlgorithm &) = default;
    CompressionAlgorithm(CompressionAlgorithm &&) = default;
    auto operator=(const CompressionAlgorithm &) -> CompressionAlgorithm & = default;
    auto operator=(CompressionAlgorithm &&) -> CompressionAlgorithm & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const CompressionAlgorithm &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const CompressionAlgorithm &other, value, other._value);

public: // accessors
    /// Get the stable raw algorithm value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert the algorithm to its stable lowercase identifier.
    [[nodiscard]] auto toString() const -> text::String;
    /// Parse an exact lowercase algorithm identifier.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<CompressionAlgorithm>;
    /// Parse an exact lowercase algorithm identifier.
    /// @throws err::ParseError If the identifier is unsupported.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> CompressionAlgorithm;
    /// Get all supported algorithms in stable order.
    [[nodiscard]] static auto all() noexcept -> std::span<const CompressionAlgorithm>;

private:
    Value _value{Lz4Block}; ///< The raw algorithm value.
};

}
