// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>
#include <string_view>

namespace profiling::conf {

/// A named ELCL document embedded into the profiling executable.
struct EmbeddedDocument {
    std::string_view name;
    std::string_view text;
};

/// The embedded parser profiling corpus.
extern const std::array<EmbeddedDocument, 4> cEmbeddedDocuments;

}
