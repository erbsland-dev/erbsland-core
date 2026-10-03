// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>
#include <erbsland/StringSet.hpp>

namespace tool::headers {

/// One published source header and its preferred and compatibility include paths.
struct PublicHeader {
    el::String source;           ///< Source-base-relative header.
    el::String canonical;        ///< Include-base-relative preferred wrapper.
    el::StringSet aliases;       ///< Additional include-base-relative wrappers.
    bool umbrella = false;       ///< Whether this is a generated directory umbrella.
    bool excludeFromAll = false; ///< Whether this header requires explicit opt-in.
};

}
