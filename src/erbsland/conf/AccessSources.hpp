// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Source.hpp"

namespace erbsland::conf {

/// The source identifiers to verify in the access function.
/// This structure names the individual source elements for the access function.
struct AccessSources {
    /// The source to verify.
    SourceIdentifierPtr source;
    /// The parent source that resolved this new source or `nullptr` if `source` is the root document.
    SourceIdentifierPtr parent;
    /// The root document. The root document is always present.
    SourceIdentifierPtr root;
};

}
