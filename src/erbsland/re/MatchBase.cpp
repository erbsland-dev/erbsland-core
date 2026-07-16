// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MatchBase.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::re {

using namespace text::literals;

auto MatchBase::begin() const -> InputPosition {
    return begin(0);
}

auto MatchBase::begin(const CaptureGroupIndex groupIndex) const -> InputPosition {
    if (!hasGroupIndex(groupIndex)) {
        throw err::ParameterError{"Group index is out of bounds."_el, "groupIndex"_el};
    }
    return _groups[static_cast<std::size_t>(groupIndex)].begin();
}

auto MatchBase::begin(const text::StringView &groupName) const -> InputPosition {
    return begin(getGroupIndex(groupName));
}

auto MatchBase::end() const -> InputPosition {
    return end(0);
}

auto MatchBase::end(const CaptureGroupIndex groupIndex) const -> InputPosition {
    if (!hasGroupIndex(groupIndex)) {
        throw err::ParameterError{"Group index is out of bounds."_el, "groupIndex"_el};
    }
    return _groups[static_cast<std::size_t>(groupIndex)].end();
}

auto MatchBase::end(const text::StringView &groupName) const -> InputPosition {
    return end(getGroupIndex(groupName));
}

auto MatchBase::range() const -> CaptureRange {
    return range(0);
}

auto MatchBase::range(const CaptureGroupIndex groupIndex) const -> CaptureRange {
    if (!hasGroupIndex(groupIndex)) {
        throw err::ParameterError{"Group index is out of bounds."_el, "groupIndex"_el};
    }
    return _groups[static_cast<std::size_t>(groupIndex)].range();
}

auto MatchBase::range(const text::StringView &groupName) const -> CaptureRange {
    return range(getGroupIndex(groupName));
}

auto MatchBase::group() const -> const CaptureGroup & {
    return group(0);
}

auto MatchBase::group(const CaptureGroupIndex groupIndex) const -> const CaptureGroup & {
    if (!hasGroupIndex(groupIndex)) {
        throw err::ParameterError{"Group index is out of bounds."_el, "groupIndex"_el};
    }
    return _groups[static_cast<std::size_t>(groupIndex)];
}

auto MatchBase::group(const text::StringView &groupName) const -> const CaptureGroup & {
    return group(getGroupIndex(groupName));
}

auto MatchBase::groupCount() const noexcept -> std::size_t {
    return _groups.size();
}

auto MatchBase::hasGroupIndex(const CaptureGroupIndex groupIndex) const noexcept -> bool {
    return static_cast<std::size_t>(groupIndex) < _groups.size();
}

auto MatchBase::hasGroupName(const text::StringView &groupName) const noexcept -> bool {
    buildGroupNameToGroupIndexMap();
    return _nameToGroupIndexMap->contains(groupName);
}

auto MatchBase::getGroupIndex(const text::StringView &groupName) const -> CaptureGroupIndex {
    buildGroupNameToGroupIndexMap();
    const auto result = _nameToGroupIndexMap->get(groupName);
    if (!result.has_value()) {
        throw err::ParameterError{"Unknown group name."_el, "groupName"_el};
    }
    return result.value();
}

void MatchBase::buildGroupNameToGroupIndexMap() const noexcept {
    std::call_once(_groupNameToGroupIndexMapInitFlag, [this]() -> void {
        auto map = text::StringCIHashMap<CaptureGroupIndex>{};
        for (std::size_t i = 0; i < _groups.size(); ++i) {
            const auto &name = _groups[i].name();
            if (!name.isEmpty()) {
                map.set(name, static_cast<CaptureGroupIndex>(i));
            }
        }
        _nameToGroupIndexMap = std::move(map);
    });
}

MatchBase::MatchBase(ConstRegExPtr regEx, std::vector<CaptureGroup> groups) noexcept :
    _regEx{std::move(regEx)}, _groups{std::move(groups)} {
}

}
