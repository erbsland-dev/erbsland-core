// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32StringEditor.hpp"
#include "U32StringEditorList_fwd.hpp"

#include "../impl/StringList.hpp"

namespace erbsland::text {

/// A list of UTF-32 strings.
using U32StringEditorList = impl::StringList<U32StringEditor>;

}
