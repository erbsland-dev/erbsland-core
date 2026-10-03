// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Subprocess.hpp"

#include "System.hpp"

#include "impl/subprocess/SubprocessEventEditor.hpp"
#include "impl/subprocess/SubprocessState.hpp"
#include "impl/subprocess/SubprocessValidation.hpp"

#include "../event/Events.hpp"

namespace erbsland::system {

Subprocess::Subprocess(std::shared_ptr<impl::SubprocessState> state) : _state{std::move(state)} {
}

Subprocess::~Subprocess() {
    _state->shutdown();
}

auto Subprocess::create(
    const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options)
    -> SubprocessPtr {
    impl::validateSubprocess(executable, arguments, options, false);
    return SubprocessPtr{new Subprocess{std::make_shared<impl::SubprocessState>(executable, arguments, options)}};
}

void Subprocess::start() {
    _state->start();
}

auto Subprocess::processId() const noexcept -> ProcessId {
    return _state->processId();
}

auto Subprocess::isRunning() -> bool {
    return _state->isRunning();
}

auto Subprocess::exitStatus() const -> std::optional<SubprocessExitStatus> {
    return _state->status();
}

auto Subprocess::wait() -> SubprocessExitStatus {
    return *_state->wait();
}

auto Subprocess::wait(time::TimeDelta timeout) -> std::optional<SubprocessExitStatus> {
    return _state->wait(timeout);
}

void Subprocess::terminate() {
    _state->terminate(false);
}

void Subprocess::kill() {
    _state->terminate(true);
}

auto Subprocess::standardOutput() const -> text::String {
    return _state->captured(false);
}

auto Subprocess::standardError() const -> text::String {
    return _state->captured(true);
}

auto Subprocess::wasStandardOutputTruncated() const noexcept -> bool {
    return _state->truncated(false);
}

auto Subprocess::wasStandardErrorTruncated() const noexcept -> bool {
    return _state->truncated(true);
}

void Subprocess::bindEvents(const event::EventsPtr &owner) {
    bindOwnerEvents(owner);
}

auto Subprocess::events() -> SubprocessEventEditor & {
    if (ownerEvents() == nullptr) {
        event::currentEvents()->get<System>().bindSubprocess(std::static_pointer_cast<Subprocess>(shared_from_this()));
    }
    auto owner = currentOwnerEvents();
    if (_editor == nullptr) {
        _editor = std::make_unique<impl::SubprocessEventEditor>(shared_from_this(), owner, _state);
    }
    return *_editor;
}

}
