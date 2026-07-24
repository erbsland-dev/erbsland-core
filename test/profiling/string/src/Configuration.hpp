// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProfileTypes.hpp"

#include <optional>

namespace app::string {

/// Optional command-line settings applied before scenario expansion.
/// @notest{Covered by string profiler CLI CTest entries.}
struct RunOverrides {
    std::optional<RunMode> mode;                          ///< Optional execution-mode override.
    std::optional<std::uint32_t> threadCount;             ///< Optional worker-count override.
    std::optional<SensitiveSelection> sensitiveSelection; ///< Optional U8 sensitivity override.
};

/// Load, validate, and expand string-profiler configuration.
/// @notest{Covered by string profiler dry-run and configuration CTest entries.}
class ConfigurationLoader final {
public:
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
