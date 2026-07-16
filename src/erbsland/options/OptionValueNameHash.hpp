// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"
#include "../text/u8/U8StringConstIterator.hpp"
#include "../util/HashHelper.hpp"

#include <cstddef>

namespace erbsland::options {

/// Hash helper for option value lookup names.
/// @tested{OptionsFrameworkTest}
class OptionValueNameHash {
public:
    /// Hash an option value lookup name.
    [[nodiscard]] auto operator()(const text::String &name) const -> std::size_t {
        std::size_t hash = 0;
        for (const auto character : name) {
            util::advanceHash(hash, character.toRawValue());
        }
        return hash;
    }
};

}
