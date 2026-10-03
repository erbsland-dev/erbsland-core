// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>

namespace tool::headers {

/// A rendered output retaining its parsed destination.
struct HeaderOutput {
    el::Path path;   ///< Absolute destination, retaining path metadata when available.
    el::String text; ///< Complete rendered output.
};

}
