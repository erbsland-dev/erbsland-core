// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WorkerPoolEventEditor.hpp"

#include "impl/worker_pool/WorkerPoolState.hpp"

#include "../err/LogicFailure.hpp"
#include "../event/CurrentEvents.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::util {

WorkerPoolEventEditor::WorkerPoolEventEditor(
    event::EventSourcePtr source, event::EventsPtr owner, impl::WorkerPoolStatePtr state) :
    event::impl::CommonEventEditor{std::move(source), std::move(owner)}, _state{std::move(state)} {
}

void WorkerPoolEventEditor::verifyOwner() const {
    using namespace text::literals;
    if (event::currentEvents() != target()) {
        throw err::LogicFailure{"Worker handlers must be edited on their owner loop."_el};
    }
}

auto WorkerPoolEventEditor::onError(ErrorFn callback) -> WorkerPoolEventEditor & {
    verifyOwner();
    _state->setError(std::move(callback));
    return *this;
}

auto WorkerPoolEventEditor::onCancelled(event::EventCallback callback) -> WorkerPoolEventEditor & {
    verifyOwner();
    _state->setCancelled(std::move(callback));
    return *this;
}

auto WorkerPoolEventEditor::onIdle(event::EventCallback callback) -> WorkerPoolEventEditor & {
    verifyOwner();
    _state->setIdle(std::move(callback));
    return *this;
}

auto WorkerPoolEventEditor::onFinal(event::EventCallback callback) -> WorkerPoolEventEditor & {
    verifyOwner();
    _state->setFinal(std::move(callback));
    return *this;
}

}
