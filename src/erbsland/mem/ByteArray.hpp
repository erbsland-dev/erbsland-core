// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteArray_fwd.hpp"
#include "ByteBuffer.hpp"
#include "ByteSpan.hpp"
#include "Endianness.hpp"

#include "impl/ByteArrayErrors.hpp"
#include "impl/ByteComparisonTools.hpp"
#include "impl/ByteDataView.hpp"
#include "impl/ByteReadTools.hpp"
#include "impl/ByteWriteTools.hpp"
#include "impl/SecureErase.hpp"
#include "impl/UnsafeByteArrayAccess_fwd.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/ByteRange.hpp"
#include "../util/impl/ComparisonHelper.hpp"
#include "../util/LoopResult.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

namespace erbsland::mem {

/// A fixed-size array of explicit byte values.
/// Default construction initializes every byte to zero.
/// Whole-array shifts and rotations treat index zero as the most-significant byte.
/// @tparam N The number of bytes.
/// @tested{ByteArrayTest}
template <std::size_t N>
class ByteArray final {
    friend class impl::UnsafeByteArrayAccess<N>;

public:
    /// The stored value type.
    using Value = Byte;

public:
    /// Create an array from exactly `N` byte values.
    /// @param bytes The byte values.
    template <typename... tBytes>
        requires(sizeof...(tBytes) == N && (std::constructible_from<Byte, tBytes> && ...))
    constexpr ByteArray(tBytes &&...bytes) noexcept : _bytes{Byte(std::forward<tBytes>(bytes))...} {}

    // defaults
    constexpr ByteArray() = default;
    ~ByteArray() = default;
    constexpr ByteArray(const ByteArray &) = default;
    constexpr ByteArray(ByteArray &&) noexcept = default;
    constexpr auto operator=(const ByteArray &) -> ByteArray & = default;
    constexpr auto operator=(ByteArray &&) noexcept -> ByteArray & = default;

public: // comparison
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_bytes, const ByteArray &other, other._bytes);
    /// Test equality without content-dependent short-circuiting.
    /// Both arrays always have the same length and every byte is inspected.
    /// @param other The byte array to compare.
    /// @return `true` if both arrays contain the same bytes.
    [[nodiscard]] auto isEqualConstTime(const ByteArray &other) const noexcept -> bool {
        return impl::ByteComparisonTools{dataView()}.isEqualConstTime(other.dataView());
    }
    /// Test equality with a borrowed sequence without content-dependent short-circuiting.
    /// Equal-length inputs always inspect every byte; a length mismatch returns immediately.
    /// @param other The borrowed byte sequence to compare.
    /// @return `true` if both sequences have the same length and contents.
    [[nodiscard]] auto isEqualConstTime(ConstByteSpan other) const noexcept -> bool {
        return impl::ByteComparisonTools{dataView()}.isEqualConstTime(impl::ByteDataView{other});
    }

public: // operators
    /// Compute the element-wise bitwise OR.
    [[nodiscard]] constexpr friend auto operator|(const ByteArray &lhs, const ByteArray &rhs) noexcept -> ByteArray {
        auto result = lhs;
        result |= rhs;
        return result;
    }
    /// Compute the element-wise bitwise AND.
    [[nodiscard]] constexpr friend auto operator&(const ByteArray &lhs, const ByteArray &rhs) noexcept -> ByteArray {
        auto result = lhs;
        result &= rhs;
        return result;
    }
    /// Compute the element-wise bitwise XOR.
    [[nodiscard]] constexpr friend auto operator^(const ByteArray &lhs, const ByteArray &rhs) noexcept -> ByteArray {
        auto result = lhs;
        result ^= rhs;
        return result;
    }
    /// Apply an element-wise bitwise OR.
    constexpr auto operator|=(const ByteArray &other) noexcept -> ByteArray & {
        for (auto i = std::size_t{0}; i < N; ++i) {
            _bytes[i] |= other._bytes[i];
        }
        return *this;
    }
    /// Apply an element-wise bitwise AND.
    constexpr auto operator&=(const ByteArray &other) noexcept -> ByteArray & {
        for (auto i = std::size_t{0}; i < N; ++i) {
            _bytes[i] &= other._bytes[i];
        }
        return *this;
    }
    /// Apply an element-wise bitwise XOR.
    constexpr auto operator^=(const ByteArray &other) noexcept -> ByteArray & {
        for (auto i = std::size_t{0}; i < N; ++i) {
            _bytes[i] ^= other._bytes[i];
        }
        return *this;
    }
    /// Invert every bit.
    [[nodiscard]] constexpr auto operator~() const noexcept -> ByteArray {
        auto result = *this;
        for (auto &byte : result._bytes) {
            byte = ~byte;
        }
        return result;
    }
    /// Shift the complete bit string left.
    [[nodiscard]] constexpr auto operator<<(const std::size_t shift) const noexcept -> ByteArray {
        return shiftedLeft(shift);
    }
    /// Shift the complete bit string right.
    [[nodiscard]] constexpr auto operator>>(const std::size_t shift) const noexcept -> ByteArray {
        return shiftedRight(shift);
    }
    /// Shift the complete bit string left in place.
    constexpr auto operator<<=(const std::size_t shift) noexcept -> ByteArray & {
        shiftLeft(shift);
        return *this;
    }
    /// Shift the complete bit string right in place.
    constexpr auto operator>>=(const std::size_t shift) noexcept -> ByteArray & {
        shiftRight(shift);
        return *this;
    }

public: // access
    /// Get the number of bytes as a byte length.
    [[nodiscard]] static constexpr auto length() noexcept -> unit::ByteLength { return unit::ByteLength::fromSizeT(N); }
    /// Get the index after the last byte.
    [[nodiscard]] static constexpr auto endIndex() noexcept -> unit::ByteIndex {
        return unit::ByteIndex::end(length());
    }
    /// Test if this array contains no bytes.
    [[nodiscard]] static constexpr auto isEmpty() noexcept -> bool { return N == 0U; }
    /// Get a byte or a default value if `index` is out of range.
    /// @param index The zero-based byte index.
    /// @param defaultValue The value returned for an invalid index.
    /// @return The stored byte or `defaultValue`.
    [[nodiscard]] constexpr auto get(const unit::ByteIndex index, const Byte defaultValue = {}) const noexcept -> Byte {
        return impl::ByteReadTools{dataView()}.get(index, defaultValue);
    }
    /// Get a byte or throw if `index` is out of range.
    /// @param index The zero-based byte index.
    /// @return The stored byte.
    /// @throws err::OutOfRangeError If `index` is out of range.
    [[nodiscard]] auto getOrThrow(const unit::ByteIndex index) const -> Byte {
        return impl::ByteReadTools{dataView()}.getOrThrow(index);
    }
    /// Invoke a callback for every byte and its optional index.
    template <typename Function>
    auto forEach(Function function) const -> util::LoopResult {
        return impl::ByteReadTools{dataView()}.forEach(std::move(function));
    }

public: // modifiers
    /// Set a byte, ignoring invalid indexes.
    constexpr void set(const unit::ByteIndex index, const Byte value) noexcept {
        impl::ByteWriteTools{ByteSpan{writableSpan()}}.set(index, value);
    }
    /// Set a byte or throw if its index is invalid.
    constexpr void setOrThrow(const unit::ByteIndex index, const Byte value) {
        impl::ByteWriteTools{ByteSpan{writableSpan()}}.setOrThrow(index, value);
    }
    /// XOR a byte value at an index, ignoring invalid indexes.
    constexpr void xorAt(const unit::ByteIndex index, const Byte value) noexcept {
        impl::ByteWriteTools{ByteSpan{writableSpan()}}.xorAt(index, value);
    }
    /// XOR a byte value at an index or throw if its index is invalid.
    /// @throws err::OutOfRangeError If `index` is invalid or outside this array.
    constexpr void xorAtOrThrow(const unit::ByteIndex index, const Byte value) {
        impl::ByteWriteTools{ByteSpan{writableSpan()}}.xorAtOrThrow(index, value);
    }
    /// Fill the array with a byte value.
    /// @param value The byte value.
    constexpr void fill(const Byte value) noexcept {
        for (auto &byte : _bytes) {
            byte = value;
        }
    }
    /// Fill a clamped byte range.
    void fill(const unit::ByteRange targetRange, const Byte value) noexcept {
        impl::ByteWriteTools{ByteSpan{writableSpan()}}.fill(targetRange, value);
    }
    /// Overwrite from the beginning with as many source bytes as fit.
    void overwrite(const ConstByteSpan source) noexcept {
        impl::ByteWriteTools{ByteSpan{writableSpan()}}.overwrite(unit::ByteRange::all(), impl::ByteDataView{source});
    }
    /// Overwrite from an index with as many source bytes as fit.
    void overwrite(const unit::ByteIndex index, const ConstByteSpan source) noexcept {
        impl::ByteWriteTools{ByteSpan{writableSpan()}}.overwrite(
            unit::ByteRange{index, unit::ByteLength::infinite()}, impl::ByteDataView{source});
    }
    /// Overwrite a clamped target range with as many source bytes as fit.
    void overwrite(const unit::ByteRange targetRange, const ConstByteSpan source) noexcept {
        impl::ByteWriteTools{ByteSpan{writableSpan()}}.overwrite(targetRange, impl::ByteDataView{source});
    }
    /// XOR every byte with an equally sized source.
    [[nodiscard]] auto xorWith(const ConstByteSpan source) noexcept -> bool {
        if (source.size() != N) {
            return false;
        }
        impl::ByteWriteTools{ByteSpan{writableSpan()}}.xorWith(unit::ByteRange::all(), impl::ByteDataView{source});
        return true;
    }
    /// XOR every byte with an equally sized source or throw if the lengths differ.
    /// @throws err::ParameterError If the lengths differ.
    void xorWithOrThrow(const ConstByteSpan source) {
        if (!xorWith(source)) {
            impl::throwByteArrayWrongLength();
        }
    }
    /// XOR a clamped range with as many source bytes as fit.
    void xorWith(const unit::ByteRange targetRange, const ConstByteSpan source) noexcept {
        impl::ByteWriteTools{ByteSpan{writableSpan()}}.xorWith(targetRange, impl::ByteDataView{source});
    }

public: // whole-array bit operations
    /// Return the complete bit string shifted left.
    /// @param shift The number of bit positions.
    /// @return The shifted array.
    [[nodiscard]] constexpr auto shiftedLeft(const std::size_t shift) const noexcept -> ByteArray {
        auto result = ByteArray{};
        if constexpr (N == 0U) {
            return result;
        }
        if (shift >= bitCount()) {
            return result;
        }
        const auto byteShift = shift / 8U;
        const auto bitShift = shift % 8U;
        for (auto destination = std::size_t{0}; destination < N; ++destination) {
            const auto source = destination + byteShift;
            if (source >= N) {
                continue;
            }
            auto value = static_cast<unsigned int>(_bytes[source].toUInt8()) << bitShift;
            if (bitShift != 0U && source + 1U < N) {
                value |= static_cast<unsigned int>(_bytes[source + 1U].toUInt8()) >> (8U - bitShift);
            }
            result._bytes[destination] = Byte::fromCroppedUInt32(value);
        }
        return result;
    }
    /// Shift the complete bit string left in place.
    /// @param shift The number of bit positions.
    constexpr void shiftLeft(const std::size_t shift) noexcept { *this = shiftedLeft(shift); }
    /// Return the complete bit string shifted right.
    /// @param shift The number of bit positions.
    /// @return The shifted array.
    [[nodiscard]] constexpr auto shiftedRight(const std::size_t shift) const noexcept -> ByteArray {
        auto result = ByteArray{};
        if constexpr (N == 0U) {
            return result;
        }
        if (shift >= bitCount()) {
            return result;
        }
        const auto byteShift = shift / 8U;
        const auto bitShift = shift % 8U;
        for (auto destination = std::size_t{0}; destination < N; ++destination) {
            if (destination < byteShift) {
                continue;
            }
            const auto source = destination - byteShift;
            auto value = static_cast<unsigned int>(_bytes[source].toUInt8()) >> bitShift;
            if (bitShift != 0U && source > 0U) {
                value |= static_cast<unsigned int>(_bytes[source - 1U].toUInt8()) << (8U - bitShift);
            }
            result._bytes[destination] = Byte::fromCroppedUInt32(value);
        }
        return result;
    }
    /// Shift the complete bit string right in place.
    /// @param shift The number of bit positions.
    constexpr void shiftRight(const std::size_t shift) noexcept { *this = shiftedRight(shift); }
    /// Return the complete bit string rotated left.
    /// @param amount The signed rotation amount.
    /// @return The rotated array.
    [[nodiscard]] constexpr auto rotatedLeft(const int amount) const noexcept -> ByteArray {
        return rotatedByNormalizedLeftAmount(normalizedLeftRotation(amount, false));
    }
    /// Rotate the complete bit string left in place.
    /// @param amount The signed rotation amount.
    constexpr void rotateLeft(const int amount) noexcept { *this = rotatedLeft(amount); }
    /// Return the complete bit string rotated right.
    /// @param amount The signed rotation amount.
    /// @return The rotated array.
    [[nodiscard]] constexpr auto rotatedRight(const int amount) const noexcept -> ByteArray {
        return rotatedByNormalizedLeftAmount(normalizedLeftRotation(amount, true));
    }
    /// Rotate the complete bit string right in place.
    /// @param amount The signed rotation amount.
    constexpr void rotateRight(const int amount) noexcept { *this = rotatedRight(amount); }

public: // per-byte bit operations
    /// Return an array with every byte shifted left independently.
    /// @param shift The number of bit positions.
    /// @return The shifted bytes.
    [[nodiscard]] constexpr auto eachByteShiftedLeft(const std::size_t shift) const noexcept -> ByteArray {
        auto result = *this;
        result.shiftEachByteLeft(shift);
        return result;
    }
    /// Shift every byte left independently in place.
    /// @param shift The number of bit positions.
    constexpr void shiftEachByteLeft(const std::size_t shift) noexcept {
        for (auto &byte : _bytes) {
            byte.shiftLeft(shift);
        }
    }
    /// Return an array with every byte shifted right independently.
    /// @param shift The number of bit positions.
    /// @return The shifted bytes.
    [[nodiscard]] constexpr auto eachByteShiftedRight(const std::size_t shift) const noexcept -> ByteArray {
        auto result = *this;
        result.shiftEachByteRight(shift);
        return result;
    }
    /// Shift every byte right independently in place.
    /// @param shift The number of bit positions.
    constexpr void shiftEachByteRight(const std::size_t shift) noexcept {
        for (auto &byte : _bytes) {
            byte.shiftRight(shift);
        }
    }
    /// Return an array with every byte rotated left independently.
    /// @param amount The signed rotation amount.
    /// @return The rotated bytes.
    [[nodiscard]] constexpr auto eachByteRotatedLeft(const int amount) const noexcept -> ByteArray {
        auto result = *this;
        result.rotateEachByteLeft(amount);
        return result;
    }
    /// Rotate every byte left independently in place.
    /// @param amount The signed rotation amount.
    constexpr void rotateEachByteLeft(const int amount) noexcept {
        for (auto &byte : _bytes) {
            byte.rotateLeft(amount);
        }
    }
    /// Return an array with every byte rotated right independently.
    /// @param amount The signed rotation amount.
    /// @return The rotated bytes.
    [[nodiscard]] constexpr auto eachByteRotatedRight(const int amount) const noexcept -> ByteArray {
        auto result = *this;
        result.rotateEachByteRight(amount);
        return result;
    }
    /// Rotate every byte right independently in place.
    /// @param amount The signed rotation amount.
    constexpr void rotateEachByteRight(const int amount) noexcept {
        for (auto &byte : _bytes) {
            byte.rotateRight(amount);
        }
    }

public: // integers
    /// Get an integer or return a default value if its byte range is invalid.
    /// @tparam T A non-boolean native integer type.
    /// @param offset The first byte index.
    /// @param endianness The byte order.
    /// @param defaultOnError The value returned for an invalid range.
    /// @return The decoded value or `defaultOnError`.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] constexpr auto getInteger(
        const unit::ByteIndex offset,
        const Endianness endianness = Endianness::Little,
        const T defaultOnError = T{}) const noexcept -> T {
        return impl::ByteReadTools{dataView()}.getInteger<T>(offset, endianness, defaultOnError);
    }
    /// Get an integer or throw if its byte range is invalid.
    /// @tparam T A non-boolean native integer type.
    /// @param offset The first byte index.
    /// @param endianness The byte order.
    /// @return The decoded value.
    /// @throws err::OutOfRangeError If the integer does not fit at `offset`.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] auto getIntegerOrThrow(
        const unit::ByteIndex offset, const Endianness endianness = Endianness::Little) const -> T {
        return impl::ByteReadTools{dataView()}.getIntegerOrThrow<T>(offset, endianness);
    }
    /// Decode an integer into an existing value.
    /// The output remains unchanged if the byte range is invalid.
    /// @tparam T A non-boolean native integer type.
    /// @param value The destination value.
    /// @param offset The first byte index.
    /// @param endianness The byte order.
    /// @return `true` on success.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] constexpr auto getIntegerInto(
        T &value, const unit::ByteIndex offset, const Endianness endianness = Endianness::Little) const noexcept
        -> bool {
        return impl::ByteReadTools{dataView()}.getIntegerInto(value, offset, endianness);
    }
    /// Store an integer in this array.
    /// The array remains unchanged if the byte range is invalid.
    /// @tparam T A non-boolean native integer type.
    /// @param offset The first byte index.
    /// @param value The integer value.
    /// @param endianness The byte order.
    /// @return `true` on success.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] constexpr auto setInteger(
        const unit::ByteIndex offset, const T value, const Endianness endianness = Endianness::Little) noexcept
        -> bool {
        return impl::ByteWriteTools{ByteSpan{writableSpan()}}.setInteger(offset, value, endianness);
    }
    /// Store an integer in this array or throw if its byte range is invalid.
    /// @tparam T A non-boolean native integer type.
    /// @param offset The first byte index.
    /// @param value The integer value.
    /// @param endianness The byte order.
    /// @throws err::OutOfRangeError If the integer does not fit at `offset`.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    constexpr void setIntegerOrThrow(
        const unit::ByteIndex offset, const T value, const Endianness endianness = Endianness::Little) {
        impl::ByteWriteTools{ByteSpan{writableSpan()}}.setIntegerOrThrow(offset, value, endianness);
    }

public: // operations
    /// Securely erase all bytes in this array.
    void secureErase() noexcept { impl::secureErase(std::as_writable_bytes(writableSpan())); }

public: // accessors
    /// Access the bytes as a read-only fixed-extent span.
    [[nodiscard]] constexpr auto span() const noexcept -> FixedConstByteSpan<N> {
        return FixedConstByteSpan<N>{_bytes};
    }
    /// Access a clamped range as a read-only byte span.
    /// @param range The byte range to access.
    /// @return The clamped span, or an empty span for an invalid range.
    [[nodiscard]] constexpr auto span(const unit::ByteRange range) const noexcept -> ConstByteSpan {
        return impl::ByteReadTools{dataView()}.span(range);
    }
    /// Access a clamped range as a read-only byte span.
    /// @param begin The first byte index.
    /// @param lengthValue The requested byte length.
    /// @return The clamped span, or an empty span for an invalid index.
    [[nodiscard]] constexpr auto span(const unit::ByteIndex begin, const unit::ByteLength lengthValue) const noexcept
        -> ConstByteSpan {
        return span(unit::ByteRange{begin, lengthValue});
    }
    /// Copy the bytes into a dynamic byte buffer.
    [[nodiscard]] auto toByteBuffer() const -> ByteBuffer { return ByteBuffer{span()}; }

public: // factories
    /// Create an array by copying a borrowed span with exactly the required size.
    /// A length mismatch is rejected without copying any bytes.
    /// @param bytes The bytes to copy.
    /// @return The copied array, or no value if `bytes` does not contain exactly `N` bytes.
    [[nodiscard]] static auto fromSpan(const ConstByteSpan bytes) noexcept -> std::optional<ByteArray> {
        if (bytes.size() != N) {
            return std::nullopt;
        }
        auto result = ByteArray{};
        result.overwrite(bytes);
        return result;
    }
    /// Create an array by copying a borrowed span with exactly the required size.
    /// @param bytes The bytes to copy.
    /// @return The copied array.
    /// @throws err::ParameterError If `bytes` does not contain exactly `N` bytes.
    [[nodiscard]] static auto fromSpanOrThrow(const ConstByteSpan bytes) -> ByteArray {
        if (const auto result = fromSpan(bytes); result.has_value()) {
            return *result;
        }
        impl::throwByteArrayWrongLength();
    }

private:
    /// Access the complete array through an internal borrowed view.
    [[nodiscard]] constexpr auto dataView() const noexcept -> impl::ByteDataView {
        return impl::ByteDataView{ConstByteSpan{_bytes}};
    }
    /// Access the complete mutable byte sequence.
    [[nodiscard]] constexpr auto writableSpan() noexcept -> ByteSpan { return ByteSpan{_bytes}; }
    /// Get the number of bits in this array.
    [[nodiscard]] static constexpr auto bitCount() noexcept -> std::size_t { return N * 8U; }
    /// Normalize a requested rotation into a left-rotation amount.
    [[nodiscard]] static constexpr auto normalizedLeftRotation(const int amount, const bool right) noexcept
        -> std::size_t {
        if constexpr (N == 0U) {
            return 0U;
        }
        auto signedAmount = static_cast<int64_t>(amount);
        if (right) {
            signedAmount = -signedAmount;
        }
        if (signedAmount >= 0) {
            return static_cast<std::size_t>(signedAmount) % bitCount();
        }
        const auto reduced = static_cast<std::size_t>(-signedAmount) % bitCount();
        return reduced == 0U ? 0U : bitCount() - reduced;
    }
    /// Rotate using an already normalized left-rotation amount.
    [[nodiscard]] constexpr auto rotatedByNormalizedLeftAmount(const std::size_t amount) const noexcept -> ByteArray {
        if constexpr (N == 0U) {
            return {};
        }
        if (amount == 0U) {
            return *this;
        }
        auto result = ByteArray{};
        const auto byteShift = amount / 8U;
        const auto bitShift = amount % 8U;
        for (auto destination = std::size_t{}; destination < N; ++destination) {
            auto source = destination + byteShift;
            if (source >= N) {
                source -= N;
            }
            auto value = static_cast<unsigned int>(_bytes[source].toUInt8()) << bitShift;
            if (bitShift != 0U) {
                const auto nextSource = source + 1U == N ? 0U : source + 1U;
                value |= static_cast<unsigned int>(_bytes[nextSource].toUInt8()) >> (8U - bitShift);
            }
            result._bytes[destination] = Byte::fromCroppedUInt32(value);
        }
        return result;
    }

private:
    std::array<Byte, N> _bytes{}; ///< The fixed byte storage.
};

template <typename... tBytes>
ByteArray(tBytes...) -> ByteArray<sizeof...(tBytes)>;

}
