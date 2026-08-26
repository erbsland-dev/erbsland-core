// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../StringMap_fwd.hpp"

#include "../../util/List_fwd.hpp"

#include <functional>

namespace erbsland::text::render {

class Value;
/// A list of render values.
using ValueList = util::List<Value>;
/// A string-keyed map of render values.
using ValueMap = StringMap<Value>;
/// A callback that lazily produces a render value.
using ValueCallbackFn = std::function<Value()>;
/// An application filter receiving the piped value followed by zero, one, or two positional arguments.
/// @tested{RenderFilterTest}
using FilterFn = std::function<Value(const ValueList &)>;

}
