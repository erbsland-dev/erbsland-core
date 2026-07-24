// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SecureErase.hpp"
#include "SharedByteDataWithFlag_fwd.hpp"
#include "SharedDataPointerTraits.hpp"

#include "../SharedData.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <new>
#include <span>
#include <type_traits>

namespace erbsland::mem::impl {

/// Compact shared storage for byte-sized data with allocation-level flags.
/// @warning This is an internal storage type. Use `ByteBlock` or `ByteBlockEditor` in application code.
/// @tparam tDataType A trivially copyable one-byte element type.
/// @tested{SharedByteDataWithFlagTest}
template <typename tDataType>
class SharedByteDataWithFlag final : public SharedData {
    static_assert(sizeof(tDataType) == 1, "SharedByteDataWithFlag requires a one-byte element type.");
    static_assert(std::is_trivially_copyable_v<tDataType>, "SharedByteDataWithFlag requires trivially copyable data.");
    static_assert(
        std::is_trivially_destructible_v<tDataType>, "SharedByteDataWithFlag requires trivially destructible data.");

public:
    using DataType = tDataType;
    using SizeType = std::uint32_t;

    static constexpr auto cSensitiveFlag = std::uint8_t{0x01};

public: // access
    [[nodiscard]] auto size() const noexcept -> SizeType { return _size; }
    void setSize(SizeType newSize) noexcept {
        if (newSize > _capacity) {
            std::terminate();
        }
        _size = newSize;
    }
    [[nodiscard]] auto capacity() const noexcept -> SizeType { return _capacity; }
    [[nodiscard]] auto data() noexcept -> DataType * { return reinterpret_cast<DataType *>(this + 1); }
    [[nodiscard]] auto data() const noexcept -> const DataType * {
        return reinterpret_cast<const DataType *>(this + 1);
    }
    [[nodiscard]] auto flags() const noexcept -> std::uint8_t { return _flags; }
    void setFlags(std::uint8_t flags) noexcept { _flags = flags; }
    void setSensitive() const noexcept { _flags |= cSensitiveFlag; }
    [[nodiscard]] auto isSensitive() const noexcept -> bool { return (_flags & cSensitiveFlag) != 0; }

public: // lifetime
    [[nodiscard]] static auto create(SizeType size, SizeType capacity, std::uint8_t flags = 0)
        -> SharedByteDataWithFlag * {
        if (size > capacity || !canAllocateWithCapacity(capacity)) {
            std::terminate();
        }
        const auto allocationSize = allocationSizeForCapacity(capacity);
        auto *memory = ::operator new(allocationSize);
        return new (memory) SharedByteDataWithFlag{size, capacity, flags};
    }
    [[nodiscard]] auto clone() const -> SharedByteDataWithFlag * {
        auto *copy = create(_size, _capacity, _flags);
        std::memcpy(copy->data(), data(), static_cast<std::size_t>(_size));
        return copy;
    }
    static void destroy(SharedByteDataWithFlag *data) noexcept {
        if (data == nullptr) {
            return;
        }
        const auto allocationSize = allocationSizeForCapacity(data->_capacity);
        const auto sensitive = data->isSensitive();
        data->~SharedByteDataWithFlag();
        if (sensitive) {
            secureErase(std::span<std::byte>{reinterpret_cast<std::byte *>(data), allocationSize});
        }
        ::operator delete(data);
    }

public: // allocation tools
    template <std::integral T>
    [[nodiscard]] static constexpr auto canAllocateWithCapacity(T capacity) noexcept -> bool {
        if constexpr (std::signed_integral<T>) {
            if (capacity < 0) {
                return false;
            }
        }
        if constexpr (sizeof(T) > sizeof(SizeType)) {
            if constexpr (std::signed_integral<T>) {
                if (static_cast<std::make_unsigned_t<T>>(capacity) > std::numeric_limits<SizeType>::max()) {
                    return false;
                }
            } else if (capacity > static_cast<T>(std::numeric_limits<SizeType>::max())) {
                return false;
            }
        }
        const auto capacityValue = static_cast<SizeType>(capacity);
        if constexpr (sizeof(SizeType) > sizeof(std::size_t)) {
            if (capacityValue > std::numeric_limits<std::size_t>::max()) {
                return false;
            }
        }
        return static_cast<std::size_t>(capacityValue) <=
            std::numeric_limits<std::size_t>::max() - allocationOverhead();
    }
    [[nodiscard]] static constexpr auto allocationOverhead() noexcept -> std::size_t {
        return sizeof(SharedByteDataWithFlag);
    }
    template <std::integral T>
    [[nodiscard]] static constexpr auto allocationSizeForCapacity(T capacity) noexcept -> std::size_t {
        if (!canAllocateWithCapacity(capacity)) {
            return std::numeric_limits<std::size_t>::max();
        }
        return allocationOverhead() + static_cast<std::size_t>(capacity);
    }

private:
    SharedByteDataWithFlag(SizeType size, SizeType capacity, std::uint8_t flags) noexcept :
        _size(size), _capacity(capacity), _flags(flags) {}

private:
    template <typename, typename>
    friend struct SharedDataPointerTraits;

private:
    SizeType _size;
    SizeType _capacity;
    mutable std::uint8_t _flags;
};

template <typename tDataType>
auto SharedDataPointerTraits<SharedByteDataWithFlag<tDataType>>::referenceCounter(Type *data) noexcept
    -> ReferenceCounter & {
    return data->_referenceCount;
}

template <typename tDataType>
auto SharedDataPointerTraits<SharedByteDataWithFlag<tDataType>>::referenceCounter(const Type *data) noexcept
    -> const ReferenceCounter & {
    return data->_referenceCount;
}

template <typename tDataType>
auto SharedDataPointerTraits<SharedByteDataWithFlag<tDataType>>::clone(const Type *data) -> Type * {
    return data->clone();
}

template <typename tDataType>
void SharedDataPointerTraits<SharedByteDataWithFlag<tDataType>>::destroy(Type *data) noexcept {
    Type::destroy(data);
}

}
