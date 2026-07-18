// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringEditor.hpp"
#include "U16StringEditorList_fwd.hpp"

#include "../impl/StringList.hpp"

namespace erbsland::text {

/// A list of UTF-16 strings.
using U16StringEditorList = impl::StringList<U16StringEditor>;

}
