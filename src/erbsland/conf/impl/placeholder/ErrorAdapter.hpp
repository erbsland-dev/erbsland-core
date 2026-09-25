// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/placeholder/ReplacerErrorCategory.hpp"
#include "../../ConfErrorCategory.hpp"

namespace erbsland::conf::impl::placeholder {

/// Return the configuration error category for a shared placeholder failure.
/// @tested{ParserPlaceholderTest}
[[nodiscard]] auto toConfErrorCategory(text::placeholder::ReplacerErrorCategory value) noexcept -> ConfErrorCategory;

}
