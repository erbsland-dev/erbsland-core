// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathContent_fwd.hpp"

#include "../Path.hpp"

namespace erbsland::path::impl {

class PathContent {
public:
    PathContent() = default;
    explicit PathContent(Path path) noexcept;

public:
    [[nodiscard]] auto path() const noexcept -> const Path &;

private:
    Path _path;
};

}
