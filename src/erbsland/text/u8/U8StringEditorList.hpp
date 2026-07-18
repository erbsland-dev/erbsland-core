// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringEditor.hpp"
#include "U8StringEditorList_fwd.hpp"

#include "../impl/StringList.hpp"

namespace erbsland::text {

/// A list of UTF-8 strings.
using U8StringEditorList = impl::StringList<U8StringEditor>;

}
