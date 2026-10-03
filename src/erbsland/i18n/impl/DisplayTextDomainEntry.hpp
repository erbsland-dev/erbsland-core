// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/StringLiteral.hpp"

namespace erbsland::i18n::impl {

/// One entry in a sorted, permanently backed display-text domain.
/// @tested{DisplayTextDataTest}
struct DisplayTextDomainEntry {
    text::StringLiteral key;  ///< The normalized final name.
    text::StringLiteral text; ///< The source text or format pattern.
};

}
