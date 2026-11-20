// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/StringList.hpp>
#include <erbsland/TimeDelta.hpp>

namespace demo {

/// One subtitle and its half-open interval on the playback timeline.
struct CaptionCue {
    el::String number;    ///< Label from the file.
    el::TimeDelta start;  ///< Offset from the beginning of playback.
    el::TimeDelta end;    ///< Offset at which the cue disappears.
    el::StringList lines; ///< Original subtitle lines.
};

}
