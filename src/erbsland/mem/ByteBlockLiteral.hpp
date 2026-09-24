// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteBlockLiteral_fwd.hpp"
#include "ByteSpan.hpp"

#include "impl/ByteBlockLiteralFactory_fwd.hpp"

#include "../unit/ByteLength.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::mem {

/// A thin wrapper around byte data with static storage duration.
/// Public construction is limited to constant evaluation so the compiler rejects references to storage that can
/// disappear. Use `fromValues()` instead of an initializer list for a short inline sequence of byte values.
/// @seedoc{/reference/mem/memory_and_byte_data}
/// @tested{ByteBlockLiteralTest}
class ByteBlockLiteral final {
    friend auto impl::unsafeCreateByteBlockLiteral(const std::uint8_t *data, std::size_t size) noexcept
        -> ByteBlockLiteral;

    /// Static storage for one compile-time byte value pack.
    template <std::uint8_t... tValues>
    struct ValueStorage final {
        static constexpr auto cData = std::array<std::uint8_t, sizeof...(tValues)>{tValues...};
    };

public:
    /// Create an empty byte block literal.
    constexpr ByteBlockLiteral() noexcept = default;
    /// Create a literal referencing a static Erbsland byte span.
    explicit consteval ByteBlockLiteral(const ConstByteSpan data) noexcept : _data{data.data()}, _size{data.size()} {}
    /// Create a literal referencing a static standard-byte span.
    explicit consteval ByteBlockLiteral(const std::span<const std::byte> data) noexcept :
        _data{data.data()}, _size{data.size()} {}
    /// Create a literal referencing a static unsigned-byte span.
    explicit consteval ByteBlockLiteral(const std::span<const std::uint8_t> data) noexcept :
        _data{data.data()}, _size{data.size()} {}
    /// Create a literal referencing a static Erbsland byte array.
    template <std::size_t N>
    explicit consteval ByteBlockLiteral(const Byte (&data)[N]) noexcept : ByteBlockLiteral{ConstByteSpan{data}} {}
    /// Create a literal referencing a static standard-byte array.
    template <std::size_t N>
    explicit consteval ByteBlockLiteral(const std::byte (&data)[N]) noexcept :
        ByteBlockLiteral{std::span<const std::byte>{data}} {}
    /// Create a literal referencing a static unsigned-byte array.
    template <std::size_t N>
    explicit consteval ByteBlockLiteral(const std::uint8_t (&data)[N]) noexcept :
        ByteBlockLiteral{std::span<const std::uint8_t>{data}} {}

    // defaults
    ~ByteBlockLiteral() = default;
    constexpr ByteBlockLiteral(const ByteBlockLiteral &) = default;
    constexpr ByteBlockLiteral(ByteBlockLiteral &&) noexcept = default;
    constexpr auto operator=(const ByteBlockLiteral &) -> ByteBlockLiteral & = default;
    constexpr auto operator=(ByteBlockLiteral &&) noexcept -> ByteBlockLiteral & = default;

public: // tests
    /// Test if this literal contains no bytes.
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool { return _size == 0U; }

public: // accessors
    /// Get the number of referenced bytes.
    [[nodiscard]] constexpr auto length() const noexcept -> unit::ByteLength {
        return unit::ByteLength::fromSizeT(_size);
    }
    /// Access the complete static byte data.
    [[nodiscard]] auto span() const noexcept -> ConstByteSpan;

public: // factory methods
    /// Create a literal backed by static storage from compile-time unsigned-byte values.
    /// This is the safe equivalent of initializer-list construction.
    template <std::uint8_t... tValues>
    [[nodiscard]] static consteval auto fromValues() noexcept -> ByteBlockLiteral {
        return ByteBlockLiteral{std::span<const std::uint8_t>{ValueStorage<tValues...>::cData}};
    }

private:
    /// Create a literal from trusted static unsigned-byte storage.
    constexpr ByteBlockLiteral(const std::uint8_t *data, const std::size_t size) noexcept : _data{data}, _size{size} {}

private:
    const void *_data{}; ///< Pointer to static byte storage.
    std::size_t _size{}; ///< Number of bytes in the storage.
};

}
