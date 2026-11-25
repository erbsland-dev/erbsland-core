// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tool/frame/ProjectFiles.hpp>

namespace tool::doc::impl {

/// Generate Doxygen-compatible headers and retain unchanged output caches.
class SourcePreprocessor final {
public:
    /// Bind the preprocessor to confined project files.
    /// @param files Project file access.
    explicit SourcePreprocessor(const frame::ProjectFiles &files);

public: // processing
    /// Transform one header without modifying its constraints.
    /// @param source Original header.
    /// @return Doxygen-compatible text.
    static auto transform(const el::String &source) -> el::String;
    /// Refresh the generated tree if inputs changed.
    /// @param force Ignore the cache marker.
    /// @return True if regenerated.
    auto run(bool force = false) -> bool;

private:
    const frame::ProjectFiles &_files; ///< Confined input and output access.
};

}
