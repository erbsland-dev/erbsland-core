// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProfileTypes.hpp"

#include <optional>

namespace app::byte {

/// A validated and expanded configuration.
/// @notest{Verified by byte profiler dry-run and smoke CTest entries.}
struct Configuration {
    RunSettings run;                   ///< Run settings.
    std::vector<Scenario> scenarios{}; ///< Expanded scenarios.

    /// Load the embedded defaults and an optional overriding ELCL file.
    /// @param path Optional user configuration.
    /// @return The effective expanded configuration.
    [[nodiscard]] static auto load(const std::optional<el::Path> &path) -> Configuration;
    /// Write the embedded default configuration.
    /// @param path Destination path.
    static void writeTemplate(const el::Path &path);
};

}
