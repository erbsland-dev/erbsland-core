// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ProfileTypes.hpp"

namespace app::regex::impl::built_in_catalog {

/// Return the generated corpus named by `name`.
/// @notest{Covered by regex profiler configuration CTest entries.}
[[nodiscard]] auto generatedCorpus(const el::String &name) -> std::optional<el::String>;

/// Return the corpus-file path named by `name`.
/// @notest{Covered by regex profiler configuration CTest entries.}
[[nodiscard]] auto file(const el::String &name) -> std::optional<el::String>;

/// Return the regular-expression pattern named by `name`.
/// @notest{Covered by regex profiler configuration CTest entries.}
[[nodiscard]] auto pattern(const el::String &name) -> std::optional<el::String>;

/// Build a string containing `count` copies of `character`.
/// @notest{Covered by regex profiler configuration CTest entries.}
[[nodiscard]] auto repeatCharacter(el::Char character, std::size_t count) -> el::String;

}
