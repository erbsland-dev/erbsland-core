// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::options {

/// The order of modules, named sections, or options in generated help.
/// @seeref{options-help-sorting}
enum class OptionHelpSorting : uint8_t {
    None,        ///< Display entries in definition order.
    Inherit,     ///< Use the parent container's sorting, or `ByLongName` at the root.
    ByLongName,  ///< Sort alphabetically by name, section title, or long option name.
    ByShortName, ///< Sort options with short names first, then use name order for remaining entries.
    ByWeight     ///< Sort by ascending help weight, breaking ties by name.
};

}
