// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/StringMap_fwd.hpp"
#include "../../util/List_fwd.hpp"

namespace erbsland::data::bson {
class BsonValue;
using BsonArray = util::List<BsonValue>;
using BsonDocument = text::StringMap<BsonValue>;
}
