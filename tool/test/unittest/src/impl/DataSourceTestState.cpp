// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DataSourceTestState.hpp"

#include <erbsland/RuntimeError.hpp>
#include <erbsland/StringFormat.hpp>

namespace tool::test::impl {

using namespace el::text::literals;

auto DataSourceTestState::shared() -> DataSourceTestState & {
    static auto state = DataSourceTestState{};
    return state;
}

void DataSourceTestState::phase(const el::String &name) {
    events.append(name);
    if (phaseAction) {
        phaseAction(name);
    }
    if (failure == name) {
        throw el::RuntimeError{el::StringFormat{"Deliberate failure in {}"_el}.build(name)};
    }
}

}
