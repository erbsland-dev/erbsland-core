// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../error/InternalError.hpp"
#include "../parser/PatternNode_fwd.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/String.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../text/StringList.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ranges>
#include <span>
#include <vector>

namespace erbsland::re::impl::node_data {

/// Base class of all node data classes.
/// @note Non-virtual, cosmetic base class for compile-time type detection.
class NodeData {};

}
