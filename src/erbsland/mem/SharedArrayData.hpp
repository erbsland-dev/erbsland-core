// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReferenceCounter.hpp"
#include "SharedArrayData_fwd.hpp"

#include "impl/SharedArrayDataTraits.hpp"
#include "impl/SharedDataPointerTraits.hpp"

#include "../math/SaturatingMath.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>

namespace erbsland::mem {

/// A class for custom implicitly/explicitly shared byte data.
/// @seedoc{/reference/mem/cow_storage}
/// @warning This is an advanced data type, meant for people extending the library.
/// Do not use it unless you understand the implications and have a specific need.
///
/// @tparam tDataType The element type stored in the trailing array.
/// @tparam tSizeType The size type, either `uint32_t` or `uint64_t`.
/// @tparam tConstructMethod Controls if and how array elements are constructed.
template <typename tDataType, impl::SharedArrayDataSizeType tSizeType, SharedArrayDataConstructMethod tConstructMethod>
class SharedArrayData final {
    static_assert(
        tConstructMethod != SharedArrayDataConstructMethod::None || std::is_trivially_copyable_v<tDataType>,
        "SharedArrayData without element construction requires a trivially copyable element type.");

public:
    /// The integer type used for size and capacity values.
    using SizeType = tSizeType;
    /// The element type stored in the trailing array.
    using DataType = tDataType;

public: // usage
    /// Get the current size of the data.
    /// @return The number of elements currently used.
    [[nodiscard]] auto size() const noexcept -> SizeType { return _size; }

    /// Set the size of the data.
    /// The size must not exceed the capacity. Violating this invariant terminates the program.
    /// @param newSize The new number of used elements.
    void setSize(SizeType newSize) noexcept {
        if (newSize > _capacity) {
            std::terminate();
        }
        _size = newSize;
    }
    /// Get the current capacity of the data.
    /// @return The number of elements allocated in the trailing storage.
    [[nodiscard]] auto capacity() const noexcept -> SizeType { return _capacity; }
    /// Access the data as a pointer to the first element.
    /// @return A mutable pointer to the first element in trailing storage.
    auto data() noexcept -> DataType * {
        // The raw element storage starts directly after the header, but `this + 1` is only aligned for
        // `SharedArrayData`. If `DataType` needs stronger alignment, advance the address to the next aligned boundary.
        // The allocation reserves `alignof(DataType) - 1` extra bytes for exactly this padding.
        const auto address = reinterpret_cast<std::uintptr_t>(this + 1);
        const auto mask = static_cast<std::uintptr_t>(alignof(DataType) - 1);
        // `alignof(T)` is always a power of two. Adding the mask and clearing its low bits rounds the address up to the
        // next multiple of `alignof(DataType)`.
        const auto alignedAddress = (address + mask) & ~mask;
        return reinterpret_cast<DataType *>(alignedAddress);
    }
    /// Access the data as a pointer to the first element.
    /// @return A const pointer to the first element in trailing storage.
    auto data() const noexcept -> const DataType * { return const_cast<SharedArrayData *>(this)->data(); }

public: // construction/destruction
    /// Create a new shared array data block.
    /// If `tConstructMethod` requests element construction, this function constructs all capacity elements. If
    /// construction throws, already constructed elements and the allocation are cleaned up before the exception is
    /// rethrown. Invalid sizes terminate the program.
    /// @param size The initial number of used elements.
    /// @param capacity The number of elements to allocate.
    /// @return A newly allocated, unreferenced array data block.
    [[nodiscard]] static auto create(SizeType size, SizeType capacity) -> SharedArrayData * {
        checkSizeRange(size, capacity);
        auto *data = allocateHeader(size, capacity);
        auto constructedCount = std::size_t{0};
        try {
            constructedCount = constructElements(data->data(), toSizeT(capacity));
        } catch (...) {
            destroyElements(data->data(), constructedCount);
            data->~SharedArrayData();
            deallocateStorage(data);
            throw;
        }
        return data;
    }
    /// Create a detached copy of this shared array data.
    /// For trivially copied raw storage, only the used range is copied. For constructed element storage, the used range
    /// is copied and the remaining capacity is default/value constructed according to `tConstructMethod`.
    /// @return A newly allocated, unreferenced copy of this array data block.
    [[nodiscard]] auto clone() const -> SharedArrayData * {
        auto *copy = allocateHeader(_size, _capacity);
        auto constructedCount = std::size_t{0};
        try {
            if constexpr (tConstructMethod == SharedArrayDataConstructMethod::None) {
                std::memcpy(copy->data(), data(), toSizeT(_size) * sizeof(DataType));
            } else {
                constructedCount = copyElements(data(), copy->data(), toSizeT(_size));
                constructedCount +=
                    constructElements(copy->data() + constructedCount, toSizeT(_capacity) - constructedCount);
            }
        } catch (...) {
            destroyElements(copy->data(), constructedCount);
            copy->~SharedArrayData();
            deallocateStorage(copy);
            throw;
        }
        return copy;
    }
    /// Destroy data created by `create` or `clone`.
    /// @param data The array data block to destroy, or `nullptr`.
    static void destroy(SharedArrayData *data) noexcept {
        if (data == nullptr) {
            return;
        }
        destroyElements(data->data(), toSizeT(data->_capacity));
        data->~SharedArrayData();
        deallocateStorage(data);
    }

public: // tools
    /// Test if an array with the given capacity can be allocated.
    /// This function checks the numeric range and allocation size arithmetic. A later allocation can still throw
    /// `std::bad_alloc` if the system cannot provide the requested memory.
    /// @param capacity The requested allocated element count.
    /// @return `true` if the capacity can be passed to `create()` without violating range or overflow preconditions.
    template <std::integral T>
    [[nodiscard]] static constexpr auto canAllocateWithCapacity(const T capacity) noexcept -> bool {
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
            } else {
                if (capacity > static_cast<T>(std::numeric_limits<SizeType>::max())) {
                    return false;
                }
            }
        }
        const auto capacityValue = static_cast<SizeType>(capacity);
        if constexpr (sizeof(SizeType) > sizeof(std::size_t)) {
            if (capacityValue > std::numeric_limits<std::size_t>::max()) {
                return false;
            }
        }
        const auto capacityCount = static_cast<std::size_t>(capacityValue);
        constexpr auto maxDataBytes = std::numeric_limits<std::size_t>::max() - allocationOverhead();
        return capacityCount <= maxDataBytes / sizeof(DataType);
    }
    /// Calculate the fixed allocation overhead before element storage.
    /// @return The header size plus the maximum padding needed to align element storage.
    [[nodiscard]] constexpr static auto allocationOverhead() noexcept -> std::size_t {
        return sizeof(SharedArrayData) + alignof(DataType) - 1;
    }
    /// Calculate the allocation size for the given capacity.
    /// @param capacity The element capacity to allocate.
    /// @return The total allocation size in bytes, including header and alignment overhead.
    template <std::integral T>
    [[nodiscard]] static constexpr auto allocationSizeForCapacity(const T capacity) noexcept -> std::size_t {
        if (!canAllocateWithCapacity(capacity)) {
            return std::numeric_limits<std::size_t>::max();
        }
        return allocationOverhead() + static_cast<std::size_t>(capacity) * sizeof(DataType);
    }

private:
    /// Check size and capacity before allocation.
    /// @param size The requested used element count.
    /// @param capacity The requested allocated element count.
    static void checkSizeRange(SizeType size, SizeType capacity) noexcept {
        if (size > capacity || !canAllocateWithCapacity(capacity)) {
            std::terminate();
        }
    }
    /// Allocate and construct only the array header.
    /// The element storage is left uninitialized here and is handled by `constructElements()` or `clone()`.
    /// @param size The used element count.
    /// @param capacity The allocated element count.
    /// @return A newly allocated header with enough trailing storage for `capacity` elements.
    [[nodiscard]] static auto allocateHeader(SizeType size, SizeType capacity) -> SharedArrayData * {
        const auto capacityCount = toSizeT(capacity);
        const auto dataBytes = capacityCount * sizeof(DataType);
        // The allocation contains the header plus the elements. Because the start of trailing storage may need to be
        // moved forward for `DataType` alignment, reserve up to `alignof(DataType) - 1` bytes of padding.
        constexpr auto headerPlusAlignment = allocationOverhead();
        if (math::willAddOverflow(headerPlusAlignment, dataBytes)) {
            std::terminate();
        }
        const auto bytes = headerPlusAlignment + dataBytes;
        auto mem = ::operator new(bytes, std::align_val_t{dataAlignment()});
        return new (mem) SharedArrayData{size, capacity};
    }
    /// Deallocate storage allocated by `allocateHeader()`.
    /// @param data The header pointer returned by `allocateHeader()`.
    static void deallocateStorage(SharedArrayData *data) noexcept {
        ::operator delete(data, std::align_val_t{dataAlignment()});
    }
    /// Construct elements according to the configured construction policy.
    /// If construction throws, this function destroys the elements it already constructed before rethrowing.
    /// @param data The first element to construct.
    /// @param count The number of elements to construct.
    /// @return The number of successfully constructed elements.
    [[nodiscard]] static auto constructElements(DataType *data, std::size_t count) -> std::size_t {
        auto constructedCount = std::size_t{0};
        try {
            if constexpr (tConstructMethod == SharedArrayDataConstructMethod::DefaultConstruct) {
                for (; constructedCount < count; ++constructedCount) {
                    new (static_cast<void *>(data + constructedCount)) DataType;
                }
            } else if constexpr (tConstructMethod == SharedArrayDataConstructMethod::ValueConstruct) {
                for (; constructedCount < count; ++constructedCount) {
                    new (static_cast<void *>(data + constructedCount)) DataType{};
                }
            }
        } catch (...) {
            destroyElements(data, constructedCount);
            throw;
        }
        return constructedCount;
    }
    /// Copy construct a range of elements.
    /// If copying throws, this function destroys the destination elements it already constructed before rethrowing.
    /// @param source The first source element.
    /// @param destination The first destination element.
    /// @param count The number of elements to copy.
    /// @return The number of successfully constructed destination elements.
    [[nodiscard]] static auto copyElements(const DataType *source, DataType *destination, std::size_t count)
        -> std::size_t {
        auto constructedCount = std::size_t{0};
        try {
            for (; constructedCount < count; ++constructedCount) {
                new (static_cast<void *>(destination + constructedCount)) DataType{source[constructedCount]};
            }
        } catch (...) {
            destroyElements(destination, constructedCount);
            throw;
        }
        return constructedCount;
    }
    /// Destroy constructed elements.
    /// This function is a no-op for raw storage mode.
    /// @param data The first element to destroy.
    /// @param count The number of elements to destroy.
    static void destroyElements(DataType *data, std::size_t count) noexcept {
        if constexpr (tConstructMethod != SharedArrayDataConstructMethod::None) {
            std::destroy_n(data, count);
        }
    }
    /// Convert the configured size type into `std::size_t` for allocation and standard-library calls.
    /// On platforms where `std::size_t` is smaller than `SizeType`, values that cannot be represented terminate the
    /// program.
    /// @param value The value to convert.
    /// @return The value as `std::size_t`.
    [[nodiscard]] static auto toSizeT(SizeType value) noexcept -> std::size_t {
        if constexpr (sizeof(SizeType) > sizeof(std::size_t)) {
            if (value > std::numeric_limits<std::size_t>::max()) {
                std::terminate();
            }
        }
        return static_cast<std::size_t>(value);
    }

private:
    /// Calculate the alignment needed by the one-block allocation.
    /// The allocation must be valid both for the header and for the trailing element array. Use the stricter of both
    /// alignments so the header placement and subsequent data alignment adjustment are always legal.
    /// @return The required allocation alignment.
    [[nodiscard]] constexpr static auto dataAlignment() noexcept -> std::size_t {
        return std::max(alignof(DataType), alignof(SharedArrayData));
    }

private:
    /// Create a header for an already allocated memory block.
    /// @param size The initial number of used elements.
    /// @param capacity The number of elements available in trailing storage.
    SharedArrayData(SizeType size, SizeType capacity) : _size(size), _capacity(capacity) {}

private:
    template <typename, typename>
    friend struct impl::SharedDataPointerTraits;

private:
    ReferenceCounter _referenceCount;
    SizeType _size;
    SizeType _capacity;

    // The actual element array is stored in the aligned trailing memory immediately after this header object.
};

}

namespace erbsland::mem::impl {

template <typename tDataType, typename tSizeType, SharedArrayDataConstructMethod tConstructMethod>
auto SharedDataPointerTraits<SharedArrayData<tDataType, tSizeType, tConstructMethod>>::referenceCounter(
    Type *data) noexcept -> ReferenceCounter & {
    return data->_referenceCount;
}

template <typename tDataType, typename tSizeType, SharedArrayDataConstructMethod tConstructMethod>
auto SharedDataPointerTraits<SharedArrayData<tDataType, tSizeType, tConstructMethod>>::referenceCounter(
    const Type *data) noexcept -> const ReferenceCounter & {
    return data->_referenceCount;
}

template <typename tDataType, typename tSizeType, SharedArrayDataConstructMethod tConstructMethod>
auto SharedDataPointerTraits<SharedArrayData<tDataType, tSizeType, tConstructMethod>>::clone(const Type *data)
    -> Type * {
    return data->clone();
}

template <typename tDataType, typename tSizeType, SharedArrayDataConstructMethod tConstructMethod>
void SharedDataPointerTraits<SharedArrayData<tDataType, tSizeType, tConstructMethod>>::destroy(Type *data) noexcept {
    Type::destroy(data);
}

}
