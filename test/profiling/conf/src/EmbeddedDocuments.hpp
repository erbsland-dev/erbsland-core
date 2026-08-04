// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EmbeddedDocument.hpp"

#include <array>

namespace app::conf {

/// Return the lazily initialized embedded parser profiling corpus.
/// @notest{Used by the parser profiling workload.}
[[nodiscard]] auto embeddedDocuments() -> const std::array<EmbeddedDocument, 4> &;

}
