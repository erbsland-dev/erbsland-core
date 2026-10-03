// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Fixture_fwd.hpp"

#include "../ProfileTypes.hpp"

#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>

namespace app::stream::impl {

/// Internal Fixture data for stream profiling.
/// @notest{Covered by stream profiler CTest entries.}
struct Fixture {
    el::Path path;                    ///< Fixture path.
    el::ByteBlock binary;             ///< Binary fixture data.
    el::String text;                  ///< Text fixture data.
    el::String expectedText;          ///< Expected text output.
    el::ByteBlock expectedRawDigest;  ///< Expected raw digest.
    el::ByteBlock expectedTextDigest; ///< Expected text digest.
    el::ByteCount rawLength{};        ///< Raw byte length.
};

}
