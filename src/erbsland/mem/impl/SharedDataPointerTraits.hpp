// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SharedByteDataWithFlag_fwd.hpp"
#include "SharedDataPointerTraits_fwd.hpp"

#include "../SharedArrayData_fwd.hpp"
#include "../SharedData.hpp"
#include "../SharedVirtualData.hpp"

#include <type_traits>

namespace erbsland::mem::impl {

/// Tests whether a type is a compact shared array data type.
template <typename tDataType>
struct IsSharedArrayData : std::false_type {};

/// Identifies a `SharedArrayData` specialization.
template <
    typename tDataType,
    typename tSizeType,
    SharedArrayDataConstructMethod tConstructMethod,
    SharedArrayDataCleanupMethod tCleanupMethod>
struct IsSharedArrayData<SharedArrayData<tDataType, tSizeType, tConstructMethod, tCleanupMethod>> : std::true_type {};

/// Tests whether a type is shared byte data with allocation flags.
template <typename tDataType>
struct IsSharedByteDataWithFlag : std::false_type {};

/// Identifies a `SharedByteDataWithFlag` specialization.
template <typename tDataType>
struct IsSharedByteDataWithFlag<SharedByteDataWithFlag<tDataType>> : std::true_type {};

/// Tests whether a type uses the regular shared-data ownership model.
template <
    typename tDataType,
    bool tIsSpecialSharedData = IsSharedArrayData<tDataType>::value || IsSharedByteDataWithFlag<tDataType>::value>
struct IsRegularSharedData : std::false_type {};

/// Identifies regular shared data derived from `SharedData`.
template <typename tDataType>
struct IsRegularSharedData<tDataType, false>
    : std::bool_constant<std::is_base_of_v<SharedData, tDataType> && !std::is_base_of_v<SharedVirtualData, tDataType>> {
};

/// Tests whether a type uses the polymorphic shared-data ownership model.
template <
    typename tDataType,
    bool tIsSpecialSharedData = IsSharedArrayData<tDataType>::value || IsSharedByteDataWithFlag<tDataType>::value>
struct IsPolymorphicSharedData : std::false_type {};

/// Identifies polymorphic shared data derived from `SharedVirtualData`.
template <typename tDataType>
struct IsPolymorphicSharedData<tDataType, false> : std::bool_constant<std::is_base_of_v<SharedVirtualData, tDataType>> {
};

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

/// Implement ownership operations for data objects derived from `SharedData`.
/// This traits base manages ordinary object data allocated with `new`. Derived classes must implement a correct copy
/// constructor for detach; the `SharedData` base resets the copied reference counter to an unreferenced state.
///
/// @tparam tDataType The concrete data type derived from `SharedData`.
template <typename tDataType>
struct RegularSharedDataPointerTraits {
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
    /// Destroy an object allocated by this traits base.
    /// @param data A non-null data object.
    static void destroy(Type *data) noexcept { delete data; }
};

/// Implement ownership operations for polymorphic data objects derived from `SharedVirtualData`.
///
/// This traits base uses the virtual `clone()` method to detach via the dynamic type. The clone must return a newly
/// allocated object compatible with the managed static type.
/// @tparam tDataType The polymorphic shared data type.
template <typename tDataType>
struct PolymorphicSharedDataPointerTraits {
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

/// Adapt ordinary shared data ownership operations to the pointer traits interface.
template <typename tDataType>
struct SharedDataPointerTraits<tDataType, std::enable_if_t<IsRegularSharedData<tDataType>::value>>
    : RegularSharedDataPointerTraits<tDataType> {};

/// Adapt polymorphic shared data ownership operations to the pointer traits interface.
template <typename tDataType>
struct SharedDataPointerTraits<tDataType, std::enable_if_t<IsPolymorphicSharedData<tDataType>::value>>
    : PolymorphicSharedDataPointerTraits<tDataType> {};

/// Traits for compact shared array data.
/// `SharedArrayData` uses one allocation that contains the header followed by aligned element storage. This means it
/// must not be destroyed with plain `delete`. The traits route clone and destroy through `SharedArrayData` so the
/// header, trailing elements, and aligned allocation are handled as one unit.
///
/// @tparam tDataType The element type stored in the shared array.
/// @tparam tSizeType The array size type, limited by `SharedArrayData`.
/// @tparam tConstructMethod The element construction policy.
template <
    typename tDataType,
    typename tSizeType,
    SharedArrayDataConstructMethod tConstructMethod,
    SharedArrayDataCleanupMethod tCleanupMethod>
struct SharedDataPointerTraits<SharedArrayData<tDataType, tSizeType, tConstructMethod, tCleanupMethod>> {
    /// The concrete managed array type.
    using Type = SharedArrayData<tDataType, tSizeType, tConstructMethod, tCleanupMethod>;

    /// Indicates that `SharedDataPointer` can manage this data type.
    static constexpr auto isSupported = true;

    /// Access the intrusive counter of a mutable array header.
    /// @param data A non-null array header.
    /// @return The reference counter embedded in the array header.
    [[nodiscard]] static auto referenceCounter(Type *data) noexcept -> ReferenceCounter &;
    /// Access the intrusive counter of a const array header.
    /// @param data A non-null array header.
    /// @return The reference counter embedded in the array header.
    [[nodiscard]] static auto referenceCounter(const Type *data) noexcept -> const ReferenceCounter &;
    /// Create an unreferenced copy for copy-on-write detach.
    /// @param data A non-null source array header.
    /// @return A new array allocated by `SharedArrayData::clone()`.
    [[nodiscard]] static auto clone(const Type *data) -> Type *;
    /// Destroy an array allocated by `SharedArrayData::create()` or `SharedArrayData::clone()`.
    /// @param data A non-null array header.
    static void destroy(Type *data) noexcept;
};

/// Traits for compact shared byte data with allocation-level flags.
template <typename tDataType>
struct SharedDataPointerTraits<SharedByteDataWithFlag<tDataType>> {
    /// The concrete managed byte-data type.
    using Type = SharedByteDataWithFlag<tDataType>;
    /// Indicates that `SharedDataPointer` can manage this data type.
    static constexpr auto isSupported = true;

    /// Access the intrusive counter of a mutable byte-data header.
    /// @param data A non-null byte-data header.
    /// @return The reference counter embedded in the byte-data header.
    [[nodiscard]] static auto referenceCounter(Type *data) noexcept -> ReferenceCounter &;
    /// Access the intrusive counter of a const byte-data header.
    /// @param data A non-null byte-data header.
    /// @return The reference counter embedded in the byte-data header.
    [[nodiscard]] static auto referenceCounter(const Type *data) noexcept -> const ReferenceCounter &;
    /// Create an unreferenced copy for copy-on-write detach.
    /// @param data A non-null source byte-data header.
    /// @return A new byte-data object allocated by its clone operation.
    [[nodiscard]] static auto clone(const Type *data) -> Type *;
    /// Destroy byte data allocated by its matching creation or clone operation.
    /// @param data A non-null byte-data header.
    static void destroy(Type *data) noexcept;
};

}
