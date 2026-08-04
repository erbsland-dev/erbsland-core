// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NamePathHelper.hpp"

#include "../../ConfError.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

auto parseNamePathLike(const NamePathLike &namePathLike) -> NamePath {
    if (std::holds_alternative<std::size_t>(namePathLike)) {
        throw conf::ConfError{ConfErrorCategory::Validation, "Index values are not allowed in this name-path"_el};
    }
    if (std::holds_alternative<Name>(namePathLike)) {
        return NamePath{std::get<Name>(namePathLike)};
    }
    if (std::holds_alternative<NamePath>(namePathLike)) {
        return std::get<NamePath>(namePathLike);
    }
    if (std::holds_alternative<text::String>(namePathLike)) {
        return NamePath::fromText(std::get<text::String>(namePathLike));
    }
    throw conf::ConfError{ConfErrorCategory::Validation, "Invalid name-path type"_el};
}

auto parseNamePathList(const std::vector<NamePathLike> &paths) -> NamePathList {
    NamePathList result;
    result.reserve(paths.size());
    for (const auto &path : paths) {
        result.emplace_back(parseNamePathLike(path));
    }
    return result;
}

}
