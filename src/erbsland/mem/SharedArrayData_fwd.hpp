// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SharedArrayDataCleanupMethod.hpp"
#include "SharedArrayDataConstructMethod.hpp"

#include "impl/SharedArrayDataTraits.hpp"

namespace erbsland::mem {

template <
    typename tDataType,
    impl::SharedArrayDataSizeType tSizeType = uint32_t,
    SharedArrayDataConstructMethod tConstructMethod = SharedArrayDataConstructMethod::None,
    SharedArrayDataCleanupMethod tCleanupMethod = SharedArrayDataCleanupMethod::None>
class SharedArrayData;

}
