// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BuiltInCatalog_fwd.hpp"

#include "../ProfileTypes.hpp"

namespace app::regex::impl {

/// Stores named built-in patterns and corpus sources.
/// @notest{Covered by regex profiler configuration CTest entries.}
class BuiltInCatalog final {
public:
    [[nodiscard]] static auto generatedCorpus(const el::String &name) -> std::optional<el::String>;
    [[nodiscard]] static auto file(const el::String &name) -> std::optional<el::String>;
    [[nodiscard]] static auto pattern(const el::String &name) -> std::optional<el::String>;

private:
    [[nodiscard]] static auto repeatCharacter(el::Char character, std::size_t count) -> el::String;
};

}
