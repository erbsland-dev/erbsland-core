// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogWriterFilter.hpp"

#include <utility>

namespace erbsland::log {

auto LogWriterFilter::setLevels(const LogLevels value) noexcept -> LogWriterFilter & {
    _levels = value;
    return *this;
}

auto LogWriterFilter::addPath(LogPath value) -> LogWriterFilter & {
    _paths.emplace_back(std::move(value));
    return *this;
}

auto LogWriterFilter::accepts(const LogLevel level, const LogPath &path) const noexcept -> bool {
    if (!_levels.isSet(level.toRawValue())) {
        return false;
    }
    if (_paths.empty()) {
        return true;
    }
    for (const auto &root : _paths) {
        if (root.contains(path)) {
            return true;
        }
    }
    return false;
}

}
