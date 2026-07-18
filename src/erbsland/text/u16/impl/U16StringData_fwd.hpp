// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/SharedArrayData_fwd.hpp"
#include "../../../mem/SharedDataPointer.hpp"
#include "../../impl/UnsafeU16StringBuffer_fwd.hpp"
#include "../../impl/UnsafeU16StringEditorAccess_fwd.hpp"

namespace erbsland::text::impl {

using U16StringData = mem::SharedArrayData<char16_t>;
using U16StringDataPtr = mem::SharedDataPointer<U16StringData>;

}
