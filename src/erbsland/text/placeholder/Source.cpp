// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Source.hpp"

#include "ReplacerError.hpp"

namespace erbsland::text::placeholder {

auto Source::validate(const String &sourceName, const String &parameter) -> bool {
    try {
        [[maybe_unused]] const auto value = resolve(sourceName, parameter);
        return true;
    } catch (const ReplacerError &) {
        return false;
    }
}

}
