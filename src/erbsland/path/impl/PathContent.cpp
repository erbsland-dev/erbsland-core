// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathContent.hpp"

#include <utility>

namespace erbsland::path::impl {

PathContent::PathContent(Path path) noexcept : _path{std::move(path)} {
}

auto PathContent::path() const noexcept -> const Path & {
    return _path;
}

}
