// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalOptionsTheme.hpp"

namespace erbsland::cterm {

auto TerminalOptionsTheme::defaultTheme() noexcept -> const TerminalOptionsTheme & {
    static const auto result = TerminalOptionsTheme{};
    return result;
}

}
