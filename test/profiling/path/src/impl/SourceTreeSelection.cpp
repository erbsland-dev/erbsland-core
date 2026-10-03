// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SourceTreeSelection.hpp"

#include <erbsland/String.hpp>
#include <erbsland/SymlinkMode.hpp>

namespace app::path::impl {

using namespace el::text::literals;

auto isSourceFile(const el::Path &path, const el::PathType type) -> bool {
    if (type != el::PathType::RegularFile) {
        return false;
    }
    const auto name = path.name();
    return name.endsWith(".hpp"_el) || name.endsWith(".cpp"_el) || name.endsWith(".tpp"_el);
}

auto shouldDescendSourceTree(const el::Path &path) -> bool {
    const auto name = path.name();
    return name != ".git"_el && name != ".cache"_el && name != "__pycache__"_el && name != "CMakeFiles"_el &&
        name != "target"_el && name != "build"_el && name != "_build"_el && !name.startsWith("cmake-build-"_el);
}

auto sourceTreeWalkOptions() -> el::PathWalkOptions {
    return el::PathWalkOptions{}
        .setSymlinkMode(el::SymlinkMode::Skip)
        .setFilter([](const el::Path &path, const el::PathType type) -> bool {
            if (type == el::PathType::Directory) {
                return shouldDescendSourceTree(path);
            }
            return isSourceFile(path, type);
        })
        .setDescendFilter([](const el::Path &path, [[maybe_unused]] const el::PathType type) -> bool {
            return shouldDescendSourceTree(path);
        });
}

}
