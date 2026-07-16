// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/SharedArrayData_fwd.hpp"
#include "../../../mem/SharedDataPointer.hpp"

namespace erbsland::text::impl {

using U32StringData = mem::SharedArrayData<char32_t>;
using U32StringDataPtr = mem::SharedDataPointer<U32StringData>;

}
