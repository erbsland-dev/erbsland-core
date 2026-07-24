// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/SharedArrayDataTraits.hpp"

#include <cstdint>

namespace erbsland::mem {

/// The method used to construct elements in shared array storage.
enum class SharedArrayDataConstructMethod : uint8_t {
    None,             ///< Leave raw trivially copyable element storage unconstructed.
    DefaultConstruct, ///< Default-construct every capacity element.
    ValueConstruct,   ///< Value-construct every capacity element.
};

/// The method used to clean up a shared array allocation before deallocation.
enum class SharedArrayDataCleanupMethod : uint8_t {
    None,        ///< Deallocate storage after normal element and header destruction.
    SecureErase, ///< Securely erase the complete allocation after destruction and before deallocation.
};

template <
    typename tDataType,
    impl::SharedArrayDataSizeType tSizeType = uint32_t,
    SharedArrayDataConstructMethod tConstructMethod = SharedArrayDataConstructMethod::None,
    SharedArrayDataCleanupMethod tCleanupMethod = SharedArrayDataCleanupMethod::None>
class SharedArrayData;

}
