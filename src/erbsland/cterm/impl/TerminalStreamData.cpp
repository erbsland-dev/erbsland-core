// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalStreamData.hpp"

#include "../Terminal.hpp"

#include <utility>

namespace erbsland::cterm::impl {

using namespace stream;

TerminalStreamData::TerminalStreamData(
    TerminalPtr streamTerminal, const BlockStyle streamStyle, const OutputStreamSettings streamSettings) :
    terminal{std::move(streamTerminal)}, style{streamStyle}, settings{streamSettings} {
    if (terminal == nullptr) {
        streamState.store(StreamState::Closed);
    }
}

void TerminalStreamData::schedule() {
    if (workInProgress || aborted.load()) {
        return;
    }
    if (queue.empty()) {
        if (streamState.load() == StreamState::Closing) {
            scheduleFlush(true, 0U);
        }
        return;
    }
    workInProgress = true;
    auto self = shared_from_this();
    auto command = queue.front();
    work.submit([self = std::move(self), command = std::move(command)]() -> void { self->performWrite(command); });
}

void TerminalStreamData::performWrite(const Command &command) {
    {
        auto terminalLock = terminal->synchronizeOutput();
        terminal->setStyle(command.style);
        if (!command.text.isEmpty()) {
            terminal->write(command.text);
        }
        if (command.lineBreak) {
            terminal->writeLineBreak();
        }
        terminal->setStyle(BlockStyle::reset());
    }
    {
        const auto lock = std::scoped_lock{mutex};
        workInProgress = false;
        if (!aborted.load()) {
            pendingBytes -= commandLength(command);
            queue.pop_front();
            schedule();
        }
    }
    condition.notify_all();
}

void TerminalStreamData::scheduleFlush(const bool closeAfterFlush, const uint64_t generation) {
    if (workInProgress || aborted.load()) {
        return;
    }
    workInProgress = true;
    auto self = shared_from_this();
    work.submit([self = std::move(self), closeAfterFlush, generation]() -> void {
        self->performFlush(closeAfterFlush, generation);
    });
}

void TerminalStreamData::performFlush(const bool closeAfterFlush, const uint64_t generation) {
    {
        auto terminalLock = terminal->synchronizeOutput();
        terminal->flush();
    }
    {
        const auto lock = std::scoped_lock{mutex};
        workInProgress = false;
        if (closeAfterFlush) {
            streamState.store(StreamState::Closed);
        } else {
            completedFlushGeneration = generation;
            schedule();
        }
    }
    condition.notify_all();
}

auto TerminalStreamData::commandLength(const Command &command) noexcept -> std::size_t {
    return command.text.length().toSizeT() + (command.lineBreak ? 1U : 0U);
}

}
