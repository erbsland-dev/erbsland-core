// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyDefinition.hpp"

#include <utility>

namespace erbsland::conf::impl {

auto KeyDefinition::create(Name name, Keys keys, const text::CaseSensitivity caseSensitivity, Location location)
    -> KeyDefinitionPtr {

    return std::make_shared<KeyDefinition>(std::move(name), std::move(keys), caseSensitivity, std::move(location));
}

KeyDefinition::KeyDefinition(
    Name name, Keys keys, const text::CaseSensitivity caseSensitivity, Location location) noexcept :
    _name{std::move(name)}, _keys{std::move(keys)}, _caseSensitivity{caseSensitivity}, _location{std::move(location)} {
}

}
