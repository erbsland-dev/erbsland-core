// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteDataView.hpp"
#include "Throw.hpp"

#include "../../unit/ByteIndex.hpp"

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

namespace erbsland::mem::impl {

/// Implements common non-comparing reads from a byte-data view.
/// @tested{ByteDataViewTest ByteBlockTest}
class ByteReadTools final {
public:
    /// Create tools for the given borrowed data view.
    explicit constexpr ByteReadTools(ByteDataView data) noexcept : _data{data} {}

public:
    /// Get a byte or a default value if the index is invalid.
    [[nodiscard]] constexpr auto get(unit::ByteIndex index, Byte defaultValue = {}) const noexcept -> Byte {
        const auto bytes = _data.dataSpan();
        if (!index.isValid() || index.toSizeT() >= bytes.size()) {
            return defaultValue;
        }
        return bytes[index.toSizeT()];
    }
    /// Get a byte or throw if the index is invalid.
    [[nodiscard]] auto getOrThrow(unit::ByteIndex index) const -> Byte {
        const auto bytes = _data.dataSpan();
        if (!index.isValid() || index.toSizeT() >= bytes.size()) {
            throwOutOfRange("Read position out of range");
        }
        return bytes[index.toSizeT()];
    }
    /// Get the bounded absolute range for a relative slice.
    [[nodiscard]] constexpr auto sliceRange(unit::ByteRange range) const noexcept -> unit::ByteRange {
        return _data.absoluteRange(range);
    }
    /// Copy the selected bytes into a byte vector.
    [[nodiscard]] auto toVector() const -> std::vector<Byte> { return copyToVector<Byte>(); }
    /// Copy the selected bytes into an unsigned-byte vector.
    [[nodiscard]] auto toUInt8Vector() const -> std::vector<uint8_t> { return copyToVector<uint8_t>(); }
    /// Copy the selected bytes into a character vector.
    [[nodiscard]] auto toCharVector() const -> std::vector<char> { return copyToVector<char>(); }

private:
    template <typename T>
    [[nodiscard]] auto copyToVector() const -> std::vector<T> {
        static_assert(sizeof(T) == sizeof(Byte));
        static_assert(std::is_trivially_copyable_v<Byte>);
        static_assert(std::is_trivially_copyable_v<T>);
        const auto bytes = _data.dataSpan();
        auto result = std::vector<T>(bytes.size());
        if (!bytes.empty()) {
            std::memcpy(result.data(), bytes.data(), bytes.size_bytes());
        }
        return result;
    }

private:
    ByteDataView _data; ///< The borrowed data operated on by these tools.
};

}
