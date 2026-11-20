// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/TimeDelta.hpp>

#include <cstddef>

namespace demo {

/// One change to the set of visible captions.
struct CueBoundary {
    el::TimeDelta offset; ///< Time on the subtitle timeline.
    bool entering;        ///< True when the cue appears, false when it disappears.
    std::size_t track;    ///< Position of the file in command-line order.
    std::size_t cue;      ///< Position of the cue in that file.
};

}
