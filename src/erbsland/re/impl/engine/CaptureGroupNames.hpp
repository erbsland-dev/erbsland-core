// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/String.hpp"
#include "../../../text/StringCIHashMap.hpp"
#include "../../CaptureGroupIndex.hpp"

#include <vector>

namespace erbsland::re::impl {

/// The list with capture group names.
using CaptureGroupNames = std::vector<text::String>;

/// Create a map from capture group names to their index.
[[nodiscard]] inline auto createCaptureGroupNameToIndexMap(const CaptureGroupNames &names) noexcept
    -> text::StringCIHashMap<CaptureGroupIndex> {
    text::StringCIHashMap<CaptureGroupIndex> map;
    auto index = CaptureGroupIndex{1};
    for (const auto &name : names) {
        if (!name.isEmpty()) {
            map.set(name, index);
        }
        ++index;
    }
    return map;
}

}
