// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <cstdint>

namespace erbsland::cterm::impl::paragraph {

/// Store the evaluated spacing tokens before a word token.
class LayoutSpacingRun final {
public:
    /// The action to take after evaluating the spacing tokens.
    enum class Action : uint8_t {
        Continue,   ///< Continue with the next word token.
        LineBreak,  ///< Break before the next word token.
        EndOfTokens ///< No word token follows the spacing run.
    };

    /// Create one evaluated spacing run.
    /// @param width The rendered width of the spacing run on this line.
    /// @param nextTokenIndex The next word token, or the next token after a forced break.
    /// @param nextTabStopIndex The next tab stop index after consuming the run.
    /// @param action The selected action for the run.
    LayoutSpacingRun(
        const int width = 0,
        const std::size_t nextTokenIndex = 0,
        const std::size_t nextTabStopIndex = 0,
        const Action action = Action::Continue) noexcept :
        width{width}, nextTokenIndex{nextTokenIndex}, nextTabStopIndex{nextTabStopIndex}, action{action} {}

    int width = 0;                    ///< The rendered width of the spacing run on this line.
    std::size_t nextTokenIndex = 0;   ///< The next word token, or the next token after a forced break.
    std::size_t nextTabStopIndex = 0; ///< The next tab stop index after consuming the run.
    Action action = Action::Continue; ///< The selected action for the run.
};

}
