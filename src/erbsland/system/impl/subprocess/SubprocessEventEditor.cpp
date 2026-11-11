// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SubprocessEventEditor.hpp"

#include "SubprocessState.hpp"

#include "../../../err/LogicFailure.hpp"
#include "../../../event/CurrentLoop.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::system::impl {

using namespace text::literals;

SubprocessEventEditor::SubprocessEventEditor(
    event::EventSourcePtr source, event::EventTargetPtr target, std::shared_ptr<SubprocessState> state) :
    system::SubprocessEventEditor{std::move(source), std::move(target)}, _state{std::move(state)} {
}

auto SubprocessEventEditor::onStarted(EventFn callback) -> system::SubprocessEventEditor & {
    verifyOwner();
    _state->setStarted(std::move(callback));
    return *this;
}

auto SubprocessEventEditor::onOutputTruncated(EventFn callback) -> system::SubprocessEventEditor & {
    verifyOwner();
    _state->setOutputTruncated(std::move(callback));
    return *this;
}

auto SubprocessEventEditor::onExit(ExitFn callback) -> system::SubprocessEventEditor & {
    verifyOwner();
    _state->setExit(std::move(callback));
    return *this;
}

auto SubprocessEventEditor::onError(ErrorFn callback) -> system::SubprocessEventEditor & {
    verifyOwner();
    _state->setError(std::move(callback));
    return *this;
}

auto SubprocessEventEditor::onFinal(EventFn callback) -> system::SubprocessEventEditor & {
    verifyOwner();
    _state->setFinal(std::move(callback));
    return *this;
}

auto SubprocessEventEditor::onStandardOutput(OutputFn callback) -> system::SubprocessEventEditor & {
    verifyOwner();
    _state->setOutput(false, std::move(callback));
    return *this;
}

auto SubprocessEventEditor::onStandardError(OutputFn callback) -> system::SubprocessEventEditor & {
    verifyOwner();
    _state->setOutput(true, std::move(callback));
    return *this;
}

void SubprocessEventEditor::verifyOwner() const {
    if (event::currentLoop() != target()) {
        throw err::LogicFailure{"Subprocess handlers must be edited on their owner loop."_el};
    }
}

}
