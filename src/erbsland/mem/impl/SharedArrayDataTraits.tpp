// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::mem::impl {

/// Access the mutable reference counter of shared array data.
template <
    typename tDataType,
    typename tSizeType,
    SharedArrayDataConstructMethod tConstructMethod,
    SharedArrayDataCleanupMethod tCleanupMethod>
auto SharedDataPointerTraits<SharedArrayData<tDataType, tSizeType, tConstructMethod, tCleanupMethod>>::referenceCounter(
    Type *data) noexcept -> ReferenceCounter & {
    return data->_referenceCount;
}

/// Access the immutable reference counter of shared array data.
template <
    typename tDataType,
    typename tSizeType,
    SharedArrayDataConstructMethod tConstructMethod,
    SharedArrayDataCleanupMethod tCleanupMethod>
auto SharedDataPointerTraits<SharedArrayData<tDataType, tSizeType, tConstructMethod, tCleanupMethod>>::referenceCounter(
    const Type *data) noexcept -> const ReferenceCounter & {
    return data->_referenceCount;
}

/// Clone shared array data into a separate allocation.
template <
    typename tDataType,
    typename tSizeType,
    SharedArrayDataConstructMethod tConstructMethod,
    SharedArrayDataCleanupMethod tCleanupMethod>
auto SharedDataPointerTraits<SharedArrayData<tDataType, tSizeType, tConstructMethod, tCleanupMethod>>::clone(
    const Type *data) -> Type * {
    return data->clone();
}

/// Destroy a shared array-data allocation.
template <
    typename tDataType,
    typename tSizeType,
    SharedArrayDataConstructMethod tConstructMethod,
    SharedArrayDataCleanupMethod tCleanupMethod>
void SharedDataPointerTraits<SharedArrayData<tDataType, tSizeType, tConstructMethod, tCleanupMethod>>::destroy(
    Type *data) noexcept {
    Type::destroy(data);
}

}
