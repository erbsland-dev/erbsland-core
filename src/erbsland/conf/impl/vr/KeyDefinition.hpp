// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/CaseSensitivity.hpp"
#include "../../Location.hpp"
#include "../../Name.hpp"
#include "../../NamePath.hpp"

#include <memory>
#include <vector>

namespace erbsland::conf::impl {

class KeyDefinition;
using KeyDefinitionPtr = std::shared_ptr<KeyDefinition>;
using KeyDefinitionList = std::vector<KeyDefinitionPtr>;

/// Defines a key entry, based on a `vr_key` definition.
class KeyDefinition {
public:
    using Keys = NamePathList;

public:
    KeyDefinition(Name name, Keys keys, text::CaseSensitivity caseSensitivity, Location location) noexcept;
    ~KeyDefinition() = default;

public:
    [[nodiscard]] static auto create(Name name, Keys keys, text::CaseSensitivity caseSensitivity, Location location)
        -> KeyDefinitionPtr;

public:
    [[nodiscard]] auto name() const noexcept -> const Name & { return _name; }
    [[nodiscard]] auto keys() const noexcept -> const Keys & { return _keys; }
    [[nodiscard]] auto caseSensitivity() const noexcept -> text::CaseSensitivity { return _caseSensitivity; }
    [[nodiscard]] auto location() const noexcept -> const Location & { return _location; }

public:
    Name _name;                                  ///< The optional name. Empty if undefined.
    Keys _keys;                                  ///< The key path(s) to the value(s).
    text::CaseSensitivity _caseSensitivity{
        text::CaseSensitivity::CaseInsensitive}; ///< If the index shall be case-sensitive.
    Location _location;
};

}
