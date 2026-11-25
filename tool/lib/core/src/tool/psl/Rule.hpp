// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>

namespace tool::psl {

/// A canonical rule with reversed labels and its exact/wildcard/exception token.
/// @tested{PublicSuffixTest PublicSuffixFullTest}
struct Rule final {
    el::String text;  ///< Reversed ASCII labels.
    unsigned token{}; ///< Exact (0), wildcard (1), or exception (2).
    auto operator<=>(const Rule &) const = default;
};

}
