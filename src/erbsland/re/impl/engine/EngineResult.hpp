// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::re::impl {

enum class EngineResult : bool {
    Continue = false, ///< Continue.
    Stop = true,      ///< Stop the thread or process.
};

}
