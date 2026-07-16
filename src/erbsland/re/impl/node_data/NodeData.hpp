// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../error/InternalError.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/String.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../text/StringView.hpp"
#include "../../../text/StringViewList.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <ranges>
#include <span>
#include <vector>

namespace erbsland::re::impl {
class PatternNode;
using PatternNodePtr = std::shared_ptr<PatternNode>;
}

namespace erbsland::re::impl::node_data {

/// Base class of all node data classes.
/// @note Non-virtual, cosmetic base class for compile-time type detection.
class NodeData {};

}
