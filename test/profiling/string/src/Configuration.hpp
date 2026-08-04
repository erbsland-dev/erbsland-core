// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RunOverrides.hpp"

namespace app::string {

/// A validated and expanded configuration.
/// @notest{Verified by string profiler configuration CTest entries.}
struct Configuration {
    RunSettings run;                   ///< Run settings.
    std::vector<Scenario> scenarios{}; ///< Expanded scenarios.

    /// Load embedded defaults and an optional overriding ELCL file.
    /// @param path Optional user configuration.
    /// @param overrides Command-line run overrides applied before expansion.
    /// @return The effective expanded configuration.
    [[nodiscard]] static auto load(const std::optional<el::Path> &path, const RunOverrides &overrides = {})
        -> Configuration;
    /// Write the embedded default configuration.
    /// @param path Destination path.
    static void writeTemplate(const el::Path &path);
};

}
