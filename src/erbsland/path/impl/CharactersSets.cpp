// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharactersSets.hpp"

#include "../../text/CharSet.hpp"

namespace erbsland::path::impl {

[[nodiscard]] auto dotCharacters() -> const text::CharSet & {
    static const auto characters = text::CharSet{U'.'};
    return characters;
}

[[nodiscard]] auto slashCharacters() -> const text::CharSet & {
    static const auto characters = text::CharSet{U'/'};
    return characters;
}

[[nodiscard]] auto pathSeparators() -> const text::CharSet & {
    static const auto characters = text::CharSet{U'/', U'\\'};
    return characters;
}

[[nodiscard]] auto invalidPathCharacters() -> const text::CharSet & {
    static const auto characters = text::CharSet{{text::Char::null(), text::Char::replacement()}};
    return characters;
}

}
