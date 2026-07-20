// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HashAlgorithm.hpp"

#include "../text/StdFormatForText.hpp"

#include <format>

template <>
struct std::formatter<erbsland::cryptology::HashAlgorithm> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::cryptology::HashAlgorithm value, std::format_context &ctx) const {
        return Base::format(value.toString(), ctx);
    }
};
