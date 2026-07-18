// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringBomMode.hpp"

#include "../mem/Byte.hpp"
#include "../mem/Endianness.hpp"
#include "../unit/ByteLength.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <span>

namespace erbsland::text {

/// A binary encoding supported by string byte conversion APIs.
/// Multi-byte encodings use either the explicit byte order named by the value, or use a byte order mark to indicate
/// the byte order when decoding. Generic UTF-16 and UTF-32 encode as little-endian.
/// @seedoc{/reference/text/string_converter}
/// @tested{StringEncodingTest}
class StringEncoding final {
public:
    /// The raw string encoding value.
    enum Value : uint8_t {
        Utf8,              ///< UTF-8 bytes without a byte order mark by default.
        Utf16,             ///< UTF-16 bytes, detect byte order from byte order mark, encode as little-endian.
        Utf16LittleEndian, ///< UTF-16 bytes, least significant byte first.
        Utf16BigEndian,    ///< UTF-16 bytes, most significant byte first.
        Utf32,             ///< UTF-32 bytes, detect byte order from byte order mark, encode as little-endian.
        Utf32LittleEndian, ///< UTF-32 bytes, least significant byte first.
        Utf32BigEndian,    ///< UTF-32 bytes, most significant byte first.
    };

public:
    /// Create a string encoding from a raw value.
    constexpr StringEncoding(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    constexpr StringEncoding() noexcept = default;
    ~StringEncoding() = default;
    StringEncoding(const StringEncoding &) = default;
    StringEncoding(StringEncoding &&) = default;
    auto operator=(const StringEncoding &) -> StringEncoding & = default;
    auto operator=(StringEncoding &&) -> StringEncoding & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const StringEncoding &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const StringEncoding &other, value, other._value);

public: // accessors
    /// Get the raw string encoding value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // tests
    /// Test if this is the UTF-8 encoding.
    [[nodiscard]] constexpr auto isUtf8() const noexcept -> bool { return _value == Utf8; }
    /// Test if this is a generic or byte-order-specific UTF-16 encoding.
    [[nodiscard]] constexpr auto isUtf16() const noexcept -> bool {
        return _value == Utf16 || _value == Utf16LittleEndian || _value == Utf16BigEndian;
    }
    /// Test if this is a generic or byte-order-specific UTF-32 encoding.
    [[nodiscard]] constexpr auto isUtf32() const noexcept -> bool {
        return _value == Utf32 || _value == Utf32LittleEndian || _value == Utf32BigEndian;
    }

public: // encoding information
    /// Resolve a generic encoding to the concrete encoding used for output.
    [[nodiscard]] constexpr auto effectiveEncoding() const noexcept -> StringEncoding {
        if (_value == Utf16) {
            return Utf16LittleEndian;
        }
        if (_value == Utf32) {
            return Utf32LittleEndian;
        }
        return *this;
    }
    /// Get the byte order used for output.
    /// UTF-8 reports little-endian as the library default, although byte order has no effect on UTF-8 data.
    [[nodiscard]] constexpr auto endianness() const noexcept -> mem::Endianness {
        const auto effective = effectiveEncoding();
        return effective == Utf16BigEndian || effective == Utf32BigEndian ? mem::Endianness::Big
                                                                          : mem::Endianness::Little;
    }
    /// Test if this encoding writes a byte order mark in the given mode.
    [[nodiscard]] constexpr auto writesBom(const StringBomMode mode) const noexcept -> bool {
        if (mode == StringBomMode::Reject) {
            return false;
        }
        return mode == StringBomMode::Require || !isUtf8();
    }
    /// Get the byte length of the byte order mark written in the given mode.
    [[nodiscard]] auto bomLength(StringBomMode mode) const noexcept -> unit::ByteLength;
    /// Get the byte order mark written in the given mode.
    /// The returned span is empty if this encoding does not write a byte order mark in the selected mode.
    [[nodiscard]] auto bomBytes(StringBomMode mode) const noexcept -> std::span<const mem::Byte>;

private:
    Value _value{Utf8}; ///< The raw string encoding value.
};

}
