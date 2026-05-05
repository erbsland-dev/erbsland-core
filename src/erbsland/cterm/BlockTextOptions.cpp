// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockTextOptions.hpp"

namespace erbsland::cterm {

auto BlockTextOptions::defaultOptions() noexcept -> const BlockTextOptions & {
    static const auto cDefaultOptions = BlockTextOptions{};
    return cDefaultOptions;
}

}
