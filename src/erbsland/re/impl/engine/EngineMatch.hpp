// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::re::impl {

/// If there is a match or not.
enum class EngineHasMatch : bool {
    No = false, ///< There is no match.
    Yes = true, ///< There is a match.
};

}
