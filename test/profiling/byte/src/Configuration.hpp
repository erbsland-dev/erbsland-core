// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProfileTypes.hpp"

#include <optional>

namespace app::byte {

/// Load, validate, and expand byte-profiler configuration.
/// @notest{Covered by byte profiler dry-run and configuration CTest entries.}
class ConfigurationLoader final {
public:
    /// Load the embedded defaults and an optional overriding ELCL file.
    /// @param path Optional user configuration.
    /// @return The effective expanded configuration.
    [[nodiscard]] static auto load(const std::optional<el::Path> &path) -> Configuration;
    /// Write the embedded default configuration.
    /// @param path Destination path.
    static void writeTemplate(const el::Path &path);
};

}
