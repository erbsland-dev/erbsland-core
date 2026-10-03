// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Program.hpp"

#include <algorithm>

namespace erbsland::text::render::impl {

using namespace unit;

auto Program::locationAt(const ByteIndex programCounter) const noexcept -> CodeLocation {
    const auto entry = std::upper_bound(
        _sourceMap.begin(),
        _sourceMap.end(),
        programCounter,
        [](const ByteIndex position, const SourceMapEntry &candidate) -> bool { return position < candidate.first; });
    if (entry == _sourceMap.begin()) {
        return {};
    }
    return std::prev(entry)->second;
}

auto Program::locationAt(const ByteIndex programCounter, std::size_t &nextSourceMapEntry) const noexcept
    -> CodeLocation {
    if (nextSourceMapEntry < _sourceMap.size() && _sourceMap[nextSourceMapEntry].first == programCounter) {
        return _sourceMap[nextSourceMapEntry++].second;
    }
    const auto entry = std::upper_bound(
        _sourceMap.begin(),
        _sourceMap.end(),
        programCounter,
        [](const ByteIndex position, const SourceMapEntry &candidate) -> bool { return position < candidate.first; });
    nextSourceMapEntry = static_cast<std::size_t>(std::distance(_sourceMap.begin(), entry));
    if (entry == _sourceMap.begin()) {
        return {};
    }
    return std::prev(entry)->second;
}

auto Program::hasInstructionAt(const ByteIndex programCounter) const noexcept -> bool {
    const auto entry = std::lower_bound(
        _sourceMap.begin(),
        _sourceMap.end(),
        programCounter,
        [](const SourceMapEntry &candidate, const ByteIndex position) -> bool { return candidate.first < position; });
    return entry != _sourceMap.end() && entry->first == programCounter;
}

}
