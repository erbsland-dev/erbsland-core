// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../FileSystemLoader.hpp"

#include "../../../path/Path.hpp"
#include "../../../util/List.hpp"

namespace erbsland::text::render::impl {

/// File-system implementation of the public layout loader.
/// @tested{FileSystemLoaderTest}
class FileSystemLoader final : public render::FileSystemLoader {
public:
    /// Create a loader for validated absolute search roots.
    FileSystemLoader(util::List<path::Path> searchPaths, [[maybe_unused]] FileSystemLoaderOptions options);

    // defaults/deletions
    ~FileSystemLoader() override = default;
    FileSystemLoader(const FileSystemLoader &) = delete;
    FileSystemLoader(FileSystemLoader &&) = delete;
    auto operator=(const FileSystemLoader &) -> FileSystemLoader & = delete;
    auto operator=(FileSystemLoader &&) -> FileSystemLoader & = delete;

public: // implement Loader
    [[nodiscard]] auto load(const String &layout) -> std::optional<LayoutSource> override;

private:
    /// Build a candidate path and reject symlink traversal.
    [[nodiscard]] auto candidate(const path::Path &searchPath, const String &layout) const -> path::Path;

private:
    util::List<path::Path> _searchPaths; ///< Absolute search roots in lookup order.
};

}
