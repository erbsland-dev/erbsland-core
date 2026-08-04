// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteBufferData_fwd.hpp"
#include "SecureErase.hpp"

#include "../Byte.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <new>
#include <span>
#include <type_traits>
#include <utility>

namespace erbsland::mem::impl {

/// Uniquely owned storage and persistent mode flags for a dynamic byte buffer.
/// @tested{ByteBufferTest}
class ByteBufferData final {
public:
    using DataType = Byte;
    using SizeType = std::size_t;

    static constexpr auto cSensitiveFlag = std::uint8_t{0x01};

public: // lifetime
    /// Create empty byte-buffer storage.
    ByteBufferData() = default;
    /// Release all allocated byte-buffer storage.
    ~ByteBufferData() { release(); }

    // defaults/deletions
    ByteBufferData(const ByteBufferData &) = delete;
    auto operator=(const ByteBufferData &) -> ByteBufferData & = delete;
    ByteBufferData(ByteBufferData &&other) noexcept :
        _data{std::exchange(other._data, nullptr)},
        _size{std::exchange(other._size, 0U)},
        _capacity{std::exchange(other._capacity, 0U)},
        _flags{std::exchange(other._flags, std::uint8_t{})} {}
    /// Replace this storage by moving from `other`.
    auto operator=(ByteBufferData &&other) noexcept -> ByteBufferData & {
        if (this != &other) {
            release();
            _data = std::exchange(other._data, nullptr);
            _size = std::exchange(other._size, 0U);
            _capacity = std::exchange(other._capacity, 0U);
            _flags = std::exchange(other._flags, std::uint8_t{});
        }
        return *this;
    }

public: // access
    /// Get the number of stored bytes.
    [[nodiscard]] auto size() const noexcept -> SizeType { return _size; }
    /// Set the number of initialized bytes.
    void setSize(const SizeType newSize) noexcept {
        if (newSize > _capacity) {
            std::terminate();
        }
        _size = newSize;
    }
    /// Get the allocation capacity in bytes.
    [[nodiscard]] auto capacity() const noexcept -> SizeType { return _capacity; }
    /// Access the mutable byte storage.
    [[nodiscard]] auto data() noexcept -> DataType * { return _data; }
    /// Access the byte storage.
    [[nodiscard]] auto data() const noexcept -> const DataType * { return _data; }
    /// Get the persistent allocation flags.
    [[nodiscard]] auto flags() const noexcept -> std::uint8_t { return _flags; }
    /// Replace the persistent allocation flags.
    void setFlags(const std::uint8_t flags) noexcept { _flags = flags; }
    /// Mark the allocation as sensitive.
    void setSensitive() const noexcept { _flags |= cSensitiveFlag; }
    /// Test whether the allocation is sensitive.
    [[nodiscard]] auto isSensitive() const noexcept -> bool { return (_flags & cSensitiveFlag) != 0; }

public: // storage
    /// Release the byte allocation while preserving the selected mode flags.
    void reset() noexcept { release(); }
    /// Exchange storage with `other`.
    void swap(ByteBufferData &other) noexcept {
        using std::swap;
        swap(_data, other._data);
        swap(_size, other._size);
        swap(_capacity, other._capacity);
        swap(_flags, other._flags);
    }
    /// Create byte storage with an initial size, capacity, and flags.
    [[nodiscard]] static auto create(
        const SizeType size, const SizeType capacity, const std::uint8_t flags = std::uint8_t{}) -> ByteBufferData {
        if (size > capacity || !canAllocateWithCapacity(capacity)) {
            std::terminate();
        }
        auto *data = capacity == 0U ? nullptr : static_cast<DataType *>(::operator new(capacity * sizeof(DataType)));
        return ByteBufferData{data, size, capacity, flags};
    }

public: // allocation tools
    /// Test whether `capacity` can be represented by this storage.
    template <std::integral T>
    [[nodiscard]] static constexpr auto canAllocateWithCapacity(const T capacity) noexcept -> bool {
        if constexpr (std::signed_integral<T>) {
            if (capacity < 0) {
                return false;
            }
        }
        if constexpr (sizeof(T) > sizeof(SizeType)) {
            if constexpr (std::signed_integral<T>) {
                return static_cast<std::make_unsigned_t<T>>(capacity) <= std::numeric_limits<SizeType>::max();
            } else {
                return capacity <= static_cast<T>(std::numeric_limits<SizeType>::max());
            }
        }
        return true;
    }
    /// Get the allocation overhead before byte storage.
    [[nodiscard]] static constexpr auto allocationOverhead() noexcept -> std::size_t { return 0U; }
    /// Get the allocation size required for `capacity` bytes.
    template <std::integral T>
    [[nodiscard]] static constexpr auto allocationSizeForCapacity(const T capacity) noexcept -> std::size_t {
        if (!canAllocateWithCapacity(capacity)) {
            return std::numeric_limits<std::size_t>::max();
        }
        return static_cast<std::size_t>(capacity);
    }

private:
    /// Initialize storage from an existing allocation.
    ByteBufferData(DataType *data, const SizeType size, const SizeType capacity, const std::uint8_t flags) noexcept :
        _data{data}, _size{size}, _capacity{capacity}, _flags{flags} {}

    /// Release the allocation while preserving its flags.
    void release() noexcept {
        if (_data != nullptr) {
            if (isSensitive()) {
                secureErase(std::as_writable_bytes(std::span{_data, _capacity}));
            }
            ::operator delete(_data);
        }
        _data = nullptr;
        _size = 0U;
        _capacity = 0U;
    }

private:
    DataType *_data{};
    SizeType _size{};
    SizeType _capacity{};
    mutable std::uint8_t _flags{};
};

}
