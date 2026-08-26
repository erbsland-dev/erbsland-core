// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileSystemLoader.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../path/PathContent.hpp"
#include "../../../path/PathInfo.hpp"
#include "../../../path/PathReadTextOptions.hpp"
#include "../../../path/SymlinkMode.hpp"
#include "../../../text/EncodingMode.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/ToString.hpp"

namespace erbsland::text::render::impl {

using namespace text::literals;

FileSystemLoader::FileSystemLoader(
    util::List<path::Path> searchPaths, [[maybe_unused]] FileSystemLoaderOptions options) :
    _searchPaths{std::move(searchPaths)} {
    if (_searchPaths.isEmpty()) {
        throw err::ParameterError{"At least one layout search path is required."_el, "searchPaths"_el};
    }
    for (auto index = unit::ItemIndex::zero(); index.isWithin(_searchPaths.count()); ++index) {
        const auto searchPath = _searchPaths.get(index);
        const auto info = searchPath.info();
        if (!searchPath.isAbsolute() || !info.isDirectory() || info.isSymlink()) {
            throw err::ParameterError{
                "Every layout search path must be an existing absolute directory and not a symbolic link."_el,
                "searchPaths"_el};
        }
        _searchPaths.set(index, info.resolvedPath());
    }
}

auto FileSystemLoader::load(const String &layout) -> std::optional<LayoutSource> {
    for (const auto &searchPath : _searchPaths) {
        const auto filePath = candidate(searchPath, layout);
        if (filePath.isEmpty()) {
            continue;
        }
        const auto info = filePath.info();
        if (!info.isRegularFile() || info.isSymlink()) {
            continue;
        }
        auto readOptions = path::PathReadTextOptions{};
        readOptions.setEncodingMode(text::EncodingMode::Strict).setSymlinkMode(path::SymlinkMode::Skip);
        auto sourceText = filePath.content().readTextOrThrow(readOptions);
        if (!sourceText.isValidUtf8()) {
            continue;
        }
        const auto revision = text::toString(static_cast<uint64_t>(sourceText.toHash()));
        return LayoutSource{std::move(sourceText), info.resolvedPath().toString(), revision};
    }
    return std::nullopt;
}

auto FileSystemLoader::candidate(const path::Path &searchPath, const String &layout) const -> path::Path {
    auto result = searchPath;
    const auto relative = path::Path{layout};
    if (!relative.isValid() || !relative.isRelative()) {
        return {};
    }
    for (const auto &element : relative.elements()) {
        result /= element;
        const auto info = result.info();
        if (info.isSymlink()) {
            return {};
        }
    }
    return result;
}

}
