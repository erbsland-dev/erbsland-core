// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogWriterFilter.hpp"

#include <utility>

namespace erbsland::log {

auto LogWriterFilter::setAcceptedLevels(const LogLevels value) noexcept -> LogWriterFilter & {
    _acceptedLevels = value;
    return *this;
}

auto LogWriterFilter::setRejectedLevels(const LogLevels value) noexcept -> LogWriterFilter & {
    _acceptedLevels = ~value;
    return *this;
}

auto LogWriterFilter::addAcceptedPath(LogPath value) -> LogWriterFilter & {
    _acceptedPaths.emplace_back(std::move(value));
    return *this;
}

auto LogWriterFilter::accepts(const LogLevel level, const LogPath &path) const noexcept -> bool {
    if (!_acceptedLevels.isSet(level.toRawValue())) {
        return false;
    }
    if (_acceptedPaths.empty()) {
        return true;
    }
    for (const auto &root : _acceptedPaths) {
        if (root.contains(path)) {
            return true;
        }
    }
    return false;
}

}
