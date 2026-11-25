// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TaskProgress.hpp"

#include <erbsland/ParameterFailure.hpp>

#include <utility>

namespace tool::frame {

using namespace el::text::literals;

void TaskProgress::beginPhase(el::String phase, const std::optional<el::ItemCount> total) const {
    if (!_state) {
        return;
    }
    const auto lock = std::lock_guard{_state->mutex};
    _state->snapshot = {std::move(phase), {}, total};
}

void TaskProgress::advance(const el::ItemCount count) const {
    if (!_state) {
        return;
    }
    const auto lock = std::lock_guard{_state->mutex};
    auto &snapshot = _state->snapshot;
    if (snapshot.total && count > *snapshot.total - snapshot.completed) {
        throw el::ParameterFailure{"Task progress exceeds the declared phase total."_el, "count"_el};
    }
    snapshot.completed += count;
}

auto TaskProgress::snapshot() const -> TaskProgressSnapshot {
    if (!_state) {
        return {};
    }
    const auto lock = std::lock_guard{_state->mutex};
    return _state->snapshot;
}

auto TaskProgress::create() -> TaskProgress {
    auto result = TaskProgress{};
    result._state = std::make_shared<State>();
    return result;
}

}
