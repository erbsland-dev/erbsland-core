// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::text::render {

/// Options for the file system layout loader.
/// @tested{FileSystemLoaderTest}
class FileSystemLoaderOptions final {
public:
    // defaults
    FileSystemLoaderOptions() = default;
    FileSystemLoaderOptions(const FileSystemLoaderOptions &) = default;
    FileSystemLoaderOptions(FileSystemLoaderOptions &&) = default;
    auto operator=(const FileSystemLoaderOptions &) -> FileSystemLoaderOptions & = default;
    auto operator=(FileSystemLoaderOptions &&) -> FileSystemLoaderOptions & = default;
    ~FileSystemLoaderOptions() = default;

public:
    // no options yet
};

}
