// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/SharedArrayData_fwd.hpp"
#include "../../../mem/SharedDataPointer.hpp"
#include "../../impl/UnsafeU8StringBuffer_fwd.hpp"
#include "../../impl/UnsafeU8StringEditorAccess_fwd.hpp"

namespace erbsland::text::impl {

using U8StringData = mem::SharedArrayData<char>;
using U8StringDataPtr = mem::SharedDataPointer<U8StringData>;

}
