// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Program.hpp"

#include <algorithm>

namespace erbsland::text::render::impl {

auto Program::locationAt(const unit::ByteIndex programCounter) const noexcept -> unit::CodeLocation {
    const auto entry = std::upper_bound(
        _sourceMap.begin(),
        _sourceMap.end(),
        programCounter,
        [](const unit::ByteIndex position, const SourceMapEntry &candidate) -> bool {
            return position < candidate.first;
        });
    if (entry == _sourceMap.begin()) {
        return {};
    }
    return std::prev(entry)->second;
}

auto Program::locationAt(const unit::ByteIndex programCounter, std::size_t &nextSourceMapEntry) const noexcept
    -> unit::CodeLocation {
    if (nextSourceMapEntry < _sourceMap.size() && _sourceMap[nextSourceMapEntry].first == programCounter) {
        return _sourceMap[nextSourceMapEntry++].second;
    }
    const auto entry = std::upper_bound(
        _sourceMap.begin(),
        _sourceMap.end(),
        programCounter,
        [](const unit::ByteIndex position, const SourceMapEntry &candidate) -> bool {
            return position < candidate.first;
        });
    nextSourceMapEntry = static_cast<std::size_t>(std::distance(_sourceMap.begin(), entry));
    if (entry == _sourceMap.begin()) {
        return {};
    }
    return std::prev(entry)->second;
}

auto Program::hasInstructionAt(const unit::ByteIndex programCounter) const noexcept -> bool {
    const auto entry = std::lower_bound(
        _sourceMap.begin(),
        _sourceMap.end(),
        programCounter,
        [](const SourceMapEntry &candidate, const unit::ByteIndex position) -> bool {
            return candidate.first < position;
        });
    return entry != _sourceMap.end() && entry->first == programCounter;
}

}
