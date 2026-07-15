// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/List.hpp"

#include <memory>

namespace erbsland::text {

class TextNode;
using TextNodePtr = std::shared_ptr<TextNode>;
using TextNodeWeakPtr = std::weak_ptr<TextNode>;
using TextNodeList = util::List<TextNodePtr>;

}
