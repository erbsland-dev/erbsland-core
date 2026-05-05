// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TextId.hpp"
#include "ZoneId.hpp"

namespace erbsland::time::tz::impl {

/// A compressed generated zone name.
/// @notest{Internal generated-data helper.}
struct ZoneName final {
    TextId text1;
    TextId text2;
    TextId text3;
    ZoneId id;
};

}
