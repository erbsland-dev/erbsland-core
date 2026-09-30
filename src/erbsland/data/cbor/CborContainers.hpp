// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CborValue_fwd.hpp"

namespace erbsland::data::cbor {
/// An ordered array of CBOR values.
using CborArray = util::List<CborValue>;
/// A string-keyed map of CBOR values.
using CborMap = text::StringMap<CborValue>;
}
