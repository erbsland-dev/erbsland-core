// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProfileTypes.hpp"

namespace app::render::corpus {

/// Catalog and loader for the deterministic layout compilation corpus.
/// @tested{render-profile-coverage}
[[nodiscard]] auto entries() -> const erbsland::List<CorpusEntry> &;

/// Load one corpus entry or the complete aggregate corpus.
/// @tested{render-profile-smoke}
[[nodiscard]] auto load(const erbsland::String &selection) -> erbsland::List<PreparedLayout>;

}
