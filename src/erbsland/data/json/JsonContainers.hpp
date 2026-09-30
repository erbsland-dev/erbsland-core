// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "JsonValue_fwd.hpp"

namespace erbsland::data::json {

/// Ordered JSON array storage.
using JsonArray = util::List<JsonValue>;
/// Ordered JSON object storage.
using JsonObject = text::StringMap<JsonValue>;

}
