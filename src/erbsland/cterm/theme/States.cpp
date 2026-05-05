// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "States.hpp"

namespace erbsland::cterm::theme {

States::States(std::initializer_list<State> states) noexcept {
    for (const auto state : states) {
        add(state);
    }
}

}
