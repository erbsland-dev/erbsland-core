// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalStreamData.hpp"

#include "../Terminal.hpp"

#include "../../stream/impl/IoService.hpp"

#include <utility>

namespace erbsland::cterm::impl {

TerminalStreamData::TerminalStreamData(
    TerminalPtr streamTerminal,
    const BlockStyle streamStyle,
    TerminalStreamSynchronizationPtr streamSynchronization,
    const stream::OutputStreamSettings streamSettings) :
    terminal{std::move(streamTerminal)},
    style{streamStyle},
    synchronization{std::move(streamSynchronization)},
    settings{streamSettings} {
    if (terminal == nullptr) {
        streamState.store(stream::StreamState::Closed);
    }
}

void TerminalStreamData::schedule() {
    if (workInProgress || aborted.load()) {
        return;
    }
    if (queue.empty()) {
        if (streamState.load() == stream::StreamState::Closing) {
            scheduleFlush(true, 0U);
        }
        return;
    }
    workInProgress = true;
    auto self = shared_from_this();
    auto command = queue.front();
    stream::impl::IoService::submitIoWork(
        [self = std::move(self), command = std::move(command)]() -> void { self->performWrite(command); });
}

void TerminalStreamData::performWrite(const Command &command) {
    auto failure = std::exception_ptr{};
    try {
        const auto terminalLock = std::scoped_lock{synchronization->_mutex};
        terminal->setStyle(command.style);
        try {
            if (!command.text.isEmpty()) {
                terminal->write(command.text);
            }
            if (command.lineBreak) {
                terminal->writeLineBreak();
            }
            terminal->setStyle(BlockStyle::reset());
        } catch (...) {
            terminal->setStyle(BlockStyle::reset());
            throw;
        }
    } catch (...) {
        failure = std::current_exception();
    }
    {
        const auto lock = std::scoped_lock{mutex};
        workInProgress = false;
        if (failure) {
            error = failure;
            streamState.store(stream::StreamState::Failed);
        } else if (!aborted.load()) {
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
    stream::impl::IoService::submitIoWork([self = std::move(self), closeAfterFlush, generation]() -> void {
        self->performFlush(closeAfterFlush, generation);
    });
}

void TerminalStreamData::performFlush(const bool closeAfterFlush, const uint64_t generation) {
    auto failure = std::exception_ptr{};
    try {
        const auto terminalLock = std::scoped_lock{synchronization->_mutex};
        terminal->flush();
    } catch (...) {
        failure = std::current_exception();
    }
    {
        const auto lock = std::scoped_lock{mutex};
        workInProgress = false;
        if (failure) {
            error = failure;
            streamState.store(stream::StreamState::Failed);
        } else if (closeAfterFlush) {
            streamState.store(stream::StreamState::Closed);
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
