// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::mem::impl {

template <
    typename tDataType,
    typename tSizeType,
    SharedArrayDataConstructMethod tConstructMethod,
    SharedArrayDataCleanupMethod tCleanupMethod>
auto SharedDataPointerTraits<SharedArrayData<tDataType, tSizeType, tConstructMethod, tCleanupMethod>>::referenceCounter(
    Type *data) noexcept -> ReferenceCounter & {
    return data->_referenceCount;
}

template <
    typename tDataType,
    typename tSizeType,
    SharedArrayDataConstructMethod tConstructMethod,
    SharedArrayDataCleanupMethod tCleanupMethod>
auto SharedDataPointerTraits<SharedArrayData<tDataType, tSizeType, tConstructMethod, tCleanupMethod>>::referenceCounter(
    const Type *data) noexcept -> const ReferenceCounter & {
    return data->_referenceCount;
}

template <
    typename tDataType,
    typename tSizeType,
    SharedArrayDataConstructMethod tConstructMethod,
    SharedArrayDataCleanupMethod tCleanupMethod>
auto SharedDataPointerTraits<SharedArrayData<tDataType, tSizeType, tConstructMethod, tCleanupMethod>>::clone(
    const Type *data) -> Type * {
    return data->clone();
}

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
