// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApiGuidelineState.hpp"

#include <erbsland/StringJoins.hpp>

namespace tool::doc::impl {

using namespace el::text::literals;

void ApiGuidelineState::issue(const el::ItemIndex index, const el::String &message) {
    issues.append(el::join(el::String::fromInteger(index.incremented().toRawValue()), ": "_el, message));
}

}
