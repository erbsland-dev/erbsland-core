// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteDataView.hpp"
#include "Throw.hpp"

#include "../ByteIntegerAccess.hpp"
#include "../ByteIntegerFormat.hpp"
#include "../Endianness.hpp"

#include "../../unit/ByteIndex.hpp"
#include "../../util/impl/LoopControl.hpp"
#include "../../util/LoopResult.hpp"
#include "../../util/LoopStatus.hpp"

#include <concepts>
#include <cstdint>
#include <cstring>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

namespace erbsland::mem::impl {

/// Implements common non-comparing reads from a byte-data view.
/// @tested{ByteDataViewTest ByteBlockTest}
class ByteReadTools final {
public:
    /// A decoded formatted integer and the number of consumed bytes.
    /// @tested{ByteReaderWriterTest}
    struct IntegerValue final {
        bool isNegative{};           ///< Whether the decoded value is negative.
        uint64_t magnitude{};        ///< The absolute decoded value.
        unit::ByteLength byteLength; ///< The encoded byte length.
    };

public:
    /// Create tools for the given borrowed data view.
    explicit constexpr ByteReadTools(ByteDataView data) noexcept : _data{data} {}

public: // accessors
    /// Test if the selected data is empty.
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool { return _data.dataSpan().empty(); }
    /// Get the selected byte length.
    [[nodiscard]] constexpr auto length() const noexcept -> unit::ByteLength { return _data.length(); }
    /// Get the index after the final selected byte.
    [[nodiscard]] constexpr auto endIndex() const noexcept -> unit::ByteIndex { return unit::ByteIndex::end(length()); }
    /// Access all selected bytes.
    [[nodiscard]] constexpr auto span() const noexcept -> ConstByteSpan { return _data.dataSpan(); }
    /// Access a clamped relative byte range.
    [[nodiscard]] constexpr auto span(unit::ByteRange range) const noexcept -> ConstByteSpan {
        return ByteDataView{_data.dataSpan(), range}.dataSpan();
    }

public: // read
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
    /// Decode a formatted integer without modifying the source.
    /// @param index The first byte index.
    /// @param format The wire format.
    /// @param endianness The byte order for fixed-width formats.
    /// @return The decoded sign, magnitude, and encoded byte length.
    /// @throws err::OutOfRangeError If the complete encoded integer is unavailable.
    /// @throws err::OverflowError If the encoded integer exceeds 64 bits.
    /// @throws err::ParseError If a variable-width integer is not minimally encoded.
    [[nodiscard]] auto getIntegerOrThrow(unit::ByteIndex index, ByteIntegerFormat format, Endianness endianness) const
        -> IntegerValue;
    /// Get a native integer or return a default value for an invalid range.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] constexpr auto getInteger(
        const unit::ByteIndex index,
        const Endianness endianness = Endianness::Little,
        const T defaultOnError = T{}) const noexcept -> T {
        return mem::getInteger<T>(_data.dataSpan(), index, endianness, defaultOnError);
    }
    /// Get a native integer or throw for an invalid range.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] constexpr auto getIntegerOrThrow(
        const unit::ByteIndex index, const Endianness endianness = Endianness::Little) const -> T {
        return mem::getIntegerOrThrow<T>(_data.dataSpan(), index, endianness);
    }
    /// Decode a native integer into an existing value.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] constexpr auto getIntegerInto(
        T &value, const unit::ByteIndex index, const Endianness endianness = Endianness::Little) const noexcept
        -> bool {
        return mem::getIntegerInto(_data.dataSpan(), value, index, endianness);
    }
    /// Get the bounded absolute range for a relative slice.
    [[nodiscard]] constexpr auto sliceRange(unit::ByteRange range) const noexcept -> unit::ByteRange {
        return _data.absoluteRange(range);
    }

public: // iteration
    /// Invoke a callback for every selected byte and its optional index.
    template <typename Function>
    auto forEach(Function function) const -> util::LoopResult {
        auto rawIndex = std::size_t{};
        for (const auto byte : _data.dataSpan()) {
            const auto status = [&]() -> util::LoopStatus {
                if constexpr (std::invocable<Function &, Byte, unit::ByteIndex>) {
                    return util::impl::invokeLoopFunction(function, byte, unit::ByteIndex::fromSizeT(rawIndex));
                } else {
                    return util::impl::invokeLoopFunction(function, byte);
                }
            }();
            if (status != util::LoopStatus::Continue) {
                return util::impl::loopStatusToResult(status);
            }
            ++rawIndex;
        }
        return util::LoopResult::Success;
    }

public: // conversion
    /// Copy the selected bytes into a byte vector.
    [[nodiscard]] auto toVector() const -> std::vector<Byte> { return copyToVector<Byte>(); }
    /// Copy the selected bytes into an unsigned-byte vector.
    [[nodiscard]] auto toUInt8Vector() const -> std::vector<uint8_t> { return copyToVector<uint8_t>(); }
    /// Copy the selected bytes into a character vector.
    [[nodiscard]] auto toCharVector() const -> std::vector<char> { return copyToVector<char>(); }

private:
    /// Copy the selected bytes into a vector with a same-sized trivial element type.
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
