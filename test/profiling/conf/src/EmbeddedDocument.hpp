// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EmbeddedDocument_fwd.hpp"

#include <erbsland/text/StringLiteral.hpp>

namespace app::conf {

/// A named ELCL document embedded into the profiling executable.
/// @notest{Plain aggregate used by the parser profiling corpus.}
struct EmbeddedDocument {
    erbsland::text::StringLiteral name; ///< Source name.
    erbsland::text::StringLiteral text; ///< ELCL document text.
};

}
