// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/SharedArrayDataTraits.hpp"

#include <cstdint>

namespace erbsland::mem {

/// The method to construct the shared array data.
enum class SharedArrayDataConstructMethod : uint8_t { None, DefaultConstruct, ValueConstruct };

template <
    typename tDataType,
    impl::SharedArrayDataSizeType tSizeType = uint32_t,
    SharedArrayDataConstructMethod tConstructMethod = SharedArrayDataConstructMethod::None>
class SharedArrayData;

}
