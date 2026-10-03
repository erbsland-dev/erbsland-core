// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharactersSets.hpp"

#include "../../text/CharSet.hpp"

namespace erbsland::path::impl {

using namespace text;

[[nodiscard]] auto dotCharacters() -> const CharSet & {
    static const auto characters = CharSet{U'.'};
    return characters;
}

[[nodiscard]] auto slashCharacters() -> const CharSet & {
    static const auto characters = CharSet{U'/'};
    return characters;
}

[[nodiscard]] auto pathSeparators() -> const CharSet & {
    static const auto characters = CharSet{U'/', U'\\'};
    return characters;
}

[[nodiscard]] auto invalidPathCharacters() -> const CharSet & {
    static const auto characters = CharSet{Char::null(), Char::replacement()};
    return characters;
}

}
