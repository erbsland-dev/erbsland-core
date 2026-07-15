// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../unit/Version.hpp"

namespace erbsland::text {

/// Get the Unicode Character Database version used by the Unicode Light layer.
/// @seedoc{/reference/text/char_range}
/// @usesunidb{Uses generated Unicode Character Database version metadata.}
[[nodiscard]] auto ucdVersion() noexcept -> unit::Version;

}
