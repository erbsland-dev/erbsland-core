// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Path.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/String.hpp>

namespace tool::headers {

/// A deferred template render retaining its parsed destination.
struct HeaderRenderJob {
    el::Path path;                     ///< Absolute output destination.
    el::String layout;                 ///< Template name.
    el::text::render::Context context; ///< Structured rendering inputs.
};

}
