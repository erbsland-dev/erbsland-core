// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/StringMap_fwd.hpp"
#include "../../util/List_fwd.hpp"

namespace erbsland::data::cbor {
class CborValue;
using CborArray = util::List<CborValue>;
using CborMap = text::StringMap<CborValue>;
}
