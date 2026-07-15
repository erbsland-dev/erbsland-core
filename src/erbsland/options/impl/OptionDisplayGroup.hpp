// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionDisplayRow.hpp"

#include "../../text/String.hpp"

#include <vector>

namespace erbsland::options::impl {

/// A group of option rows with a shared help heading.
/// @tested{OptionDocumentTest}
struct OptionDisplayGroup final {
    text::String title;                 ///< The group title shown as a heading.
    std::vector<OptionDisplayRow> rows; ///< The sorted rows in this group.
};

}
