// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DisplayTextDomainEntry.hpp"

#include <span>

namespace erbsland::i18n::impl {

/// A sorted domain whose entries have permanent backing storage.
/// @tested{DisplayTextDataTest}
struct DisplayTextDomain {
    text::StringLiteral name;                        ///< Normalized domain path, or empty for root entries.
    std::span<const DisplayTextDomainEntry> entries; ///< Entries sorted by normalized final name.
};

}
