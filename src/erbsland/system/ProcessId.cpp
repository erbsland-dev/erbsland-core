// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProcessId.hpp"

#include "../text/Literals.hpp"
#include "../text/String.hpp"
#include "../text/StringFormat.hpp"

namespace erbsland::system {

using namespace text::literals;

auto ProcessId::toString() const -> text::String {
    if (!isValid()) {
        return {};
    }
    static const auto cFormat = text::StringFormat{"{}"_el};
    return cFormat.build(_value);
}

}
