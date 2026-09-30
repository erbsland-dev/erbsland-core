// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BsonValue_fwd.hpp"

namespace erbsland::data::bson {
/// An ordered BSON array.
using BsonArray = util::List<BsonValue>;
/// A string-keyed BSON document.
using BsonDocument = text::StringMap<BsonValue>;
}
