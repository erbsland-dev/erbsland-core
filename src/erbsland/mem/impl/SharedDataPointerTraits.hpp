// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../SharedArrayData.hpp"
#include "../SharedData.hpp"
#include "../SharedVirtualData.hpp"

#include <type_traits>

namespace erbsland::mem::impl {

/// The default traits for types that are not supported by `SharedDataPointer`.
/// `SharedDataPointer` uses this traits class as a narrow bridge between pointer ownership logic and the concrete data
/// storage model. The primary template deliberately rejects all types. Add a specialization only when a data type has a
/// stable intrusive reference counter, a safe clone operation for detach, and a matching destroy operation for the
/// allocation strategy used by that type.
///
/// Maintenance guidance:
/// - `referenceCounter()` must return the exact intrusive counter owned by the data instance.
/// - `clone()` must return a newly allocated, unreferenced object. `SharedDataPointer` will add the first reference.
/// - `destroy()` must match the allocation function used by `clone()` and by the public factory for the type.
/// - All functions are called only with non-null pointers by `SharedDataPointer`; keep null handling at pointer level.
///
/// @tparam tDataType The candidate data type.
/// @tparam tEnable SFINAE hook for specializations.
template <typename tDataType, typename tEnable = void>
struct SharedDataPointerTraits {
    /// Indicates if this data type can be managed by `SharedDataPointer`.
    static constexpr auto isSupported = false;
};

/// Traits for data objects derived from `SharedData`.
/// This specialization manages ordinary object data allocated with `new`. Derived classes must implement a correct copy
/// constructor for detach; the `SharedData` base resets the copied reference counter to an unreferenced state.
///
/// @tparam tDataType The concrete data type derived from `SharedData`.
template <typename tDataType>
struct SharedDataPointerTraits<
    tDataType,
    std::enable_if_t<std::is_base_of_v<SharedData, tDataType> && !std::is_base_of_v<SharedVirtualData, tDataType>>> {
    /// The concrete managed type.
    using Type = tDataType;

    /// Indicates that `SharedDataPointer` can manage this data type.
    static constexpr auto isSupported = true;

    /// Access the intrusive counter of a mutable data object.
    /// @param data A non-null data object.
    /// @return The reference counter embedded in the data object.
    [[nodiscard]] static auto referenceCounter(Type *data) noexcept -> ReferenceCounter & {
        return data->_referenceCount;
    }
    /// Access the intrusive counter of a const data object.
    /// @param data A non-null data object.
    /// @return The reference counter embedded in the data object.
    [[nodiscard]] static auto referenceCounter(const Type *data) noexcept -> const ReferenceCounter & {
        return data->_referenceCount;
    }
    /// Create an unreferenced copy for copy-on-write detach.
    /// @param data A non-null source data object.
    /// @return A new object allocated with `new`.
    [[nodiscard]] static auto clone(const Type *data) -> Type * { return new Type{*data}; }
    /// Destroy an object allocated by this traits specialization.
    /// @param data A non-null data object.
    static void destroy(Type *data) noexcept { delete data; }
};

/// Traits for polymorphic data objects derived from `SharedVirtualData`.
///
/// This specialization uses the virtual `clone()` method to detach via the dynamic type. The clone must return a newly
/// allocated object compatible with the managed static type.
/// @tparam tDataType The polymorphic shared data type.
template <typename tDataType>
struct SharedDataPointerTraits<tDataType, std::enable_if_t<std::is_base_of_v<SharedVirtualData, tDataType>>> {
    /// The concrete managed type.
    using Type = tDataType;

    /// Indicates that `SharedDataPointer` can manage this data type.
    static constexpr auto isSupported = true;

    /// Access the intrusive counter of a mutable data object.
    [[nodiscard]] static auto referenceCounter(Type *data) noexcept -> ReferenceCounter & {
        return data->_referenceCount;
    }
    /// Access the intrusive counter of a const data object.
    [[nodiscard]] static auto referenceCounter(const Type *data) noexcept -> const ReferenceCounter & {
        return data->_referenceCount;
    }
    /// Create an unreferenced polymorphic copy for copy-on-write detach.
    [[nodiscard]] static auto clone(const Type *data) -> Type * { return static_cast<Type *>(data->clone()); }
    /// Destroy an object allocated by the virtual clone implementation.
    static void destroy(Type *data) noexcept { delete data; }
};

/// Traits for compact shared array data.
/// `SharedArrayData` uses one allocation that contains the header followed by aligned element storage. This means it
/// must not be destroyed with plain `delete`. The traits route clone and destroy through `SharedArrayData` so the
/// header, trailing elements, and aligned allocation are handled as one unit.
///
/// @tparam tDataType The element type stored in the shared array.
/// @tparam tSizeType The array size type, limited by `SharedArrayData`.
/// @tparam tConstructMethod The element construction policy.
template <typename tDataType, typename tSizeType, SharedArrayDataConstructMethod tConstructMethod>
struct SharedDataPointerTraits<SharedArrayData<tDataType, tSizeType, tConstructMethod>> {
    /// The concrete managed array type.
    using Type = SharedArrayData<tDataType, tSizeType, tConstructMethod>;

    /// Indicates that `SharedDataPointer` can manage this data type.
    static constexpr auto isSupported = true;

    /// Access the intrusive counter of a mutable array header.
    /// @param data A non-null array header.
    /// @return The reference counter embedded in the array header.
    [[nodiscard]] static auto referenceCounter(Type *data) noexcept -> ReferenceCounter & {
        return data->_referenceCount;
    }
    /// Access the intrusive counter of a const array header.
    /// @param data A non-null array header.
    /// @return The reference counter embedded in the array header.
    [[nodiscard]] static auto referenceCounter(const Type *data) noexcept -> const ReferenceCounter & {
        return data->_referenceCount;
    }
    /// Create an unreferenced copy for copy-on-write detach.
    /// @param data A non-null source array header.
    /// @return A new array allocated by `SharedArrayData::clone()`.
    [[nodiscard]] static auto clone(const Type *data) -> Type * { return data->clone(); }
    /// Destroy an array allocated by `SharedArrayData::create()` or `SharedArrayData::clone()`.
    /// @param data A non-null array header.
    static void destroy(Type *data) noexcept { Type::destroy(data); }
};

}
