// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Fixture_fwd.hpp"

#include "../ProfileTypes.hpp"

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
    std::uint64_t rawLength{};        ///< Raw byte length.
};

}
