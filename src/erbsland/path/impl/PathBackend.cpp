// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathBackend.hpp"

#include "PathInfoCache.hpp"

#include "../Path.hpp"

#include <mutex>
#include <utility>

namespace erbsland::path::impl {

auto PathBackend::loadResolvedInfoOrThrow(
    const Path &path, [[maybe_unused]] const Path &resolvedPath, const PathInfoParts parts) const -> PathInfoData {
    return loadInfoOrThrow(path, parts);
}

auto PathBackend::directoryEntryPath(const Path &base, const text::String &name) noexcept -> Path {
    if (base.isEmpty()) {
        return {};
    }
    auto data = PathData::createJoined(*base._data, name);
    return Path{PathDataPtr{data.release()}};
}

auto PathBackend::pathWithoutInfo(const Path &path) noexcept -> Path {
    if (path.isEmpty()) {
        return {};
    }
    return Path{PathDataPtr{new PathData{*path._data}}};
}

void PathBackend::preloadInfo(Path &path, PathInfoData data) {
    if (path.isEmpty()) {
        return;
    }
    const auto cache = path._data->infoCache();
    const auto lock = std::scoped_lock{cache->mutex};
    cache->data = std::move(data);
}

void PathBackend::invalidateInfo(const Path &path) {
    if (path.isEmpty()) {
        return;
    }
    const auto cache = path._data->infoCache();
    const auto lock = std::scoped_lock{cache->mutex};
    cache->data = {};
}

}
