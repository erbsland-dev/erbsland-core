// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProfileTypes.hpp"

#include <optional>

namespace app::stream {

/// The effective validated and expanded configuration.
/// @notest{Covered through command-line dry runs and profiler smoke tests.}
struct Configuration {
    RunSettings run;                 ///< Complete run settings.
    std::vector<Scenario> scenarios; ///< Expanded scenarios.

    /// Load the embedded default configuration or a user configuration.
    /// @param path Optional user configuration path.
    /// @return The validated and expanded configuration.
    [[nodiscard]] static auto load(const std::optional<el::Path> &path) -> Configuration;
    /// Write the embedded default configuration.
    /// @param path Destination path.
    static void writeTemplate(const el::Path &path);
};

}
