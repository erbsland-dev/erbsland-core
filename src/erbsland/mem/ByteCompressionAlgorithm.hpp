// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../unit/ByteLength.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace erbsland::mem {

/// A byte-compression algorithm supported by the library.
/// Raw algorithm values are stable because they are stored in compression envelopes.
/// @seedoc{/reference/mem/byte_compression}
/// @tested{ByteCompressionTest}
class ByteCompressionAlgorithm final {
public:
    /// The raw compression algorithm value.
    enum Value : uint8_t {
        Lz4Block = 1U, ///< Standard raw LZ4 block compression.
    };

public:
    /// Create the default LZ4 block algorithm.
    constexpr ByteCompressionAlgorithm() noexcept = default;
    /// Create an algorithm from its raw value.
    constexpr ByteCompressionAlgorithm(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~ByteCompressionAlgorithm() = default;
    ByteCompressionAlgorithm(const ByteCompressionAlgorithm &) = default;
    ByteCompressionAlgorithm(ByteCompressionAlgorithm &&) = default;
    auto operator=(const ByteCompressionAlgorithm &) -> ByteCompressionAlgorithm & = default;
    auto operator=(ByteCompressionAlgorithm &&) -> ByteCompressionAlgorithm & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const ByteCompressionAlgorithm &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(
        const Value value, const ByteCompressionAlgorithm &other, value, other._value);

public: // accessors
    /// Get the stable raw algorithm value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }
    /// Calculate an upper bound for a raw compressed block.
    /// @throws err::OutOfRangeError If `length` is infinite or the bound is not representable.
    [[nodiscard]] auto maximumCompressedLength(unit::ByteLength length) const -> unit::ByteLength;

public: // conversion
    /// Convert the algorithm to its stable lowercase identifier.
    [[nodiscard]] auto toString() const -> text::String;
    /// Parse an exact lowercase algorithm identifier.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<ByteCompressionAlgorithm>;
    /// Parse an exact lowercase algorithm identifier.
    /// @throws err::ParseError If the identifier is unsupported.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> ByteCompressionAlgorithm;
    /// Get all supported algorithms in stable order.
    [[nodiscard]] static auto all() noexcept -> std::span<const ByteCompressionAlgorithm>;

private:
    Value _value{Lz4Block}; ///< The raw algorithm value.
};

}
