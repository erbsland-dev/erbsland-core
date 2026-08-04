// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../StringMap_fwd.hpp"

#include "../../util/List_fwd.hpp"

namespace erbsland::text::json {

class JsonValue;
using JsonArray = util::List<JsonValue>;
using JsonObject = StringMap<JsonValue>;

}
