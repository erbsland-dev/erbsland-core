// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardTextOutputStreamData.hpp"

#include "../../err/RuntimeError.hpp"

#include <utility>

namespace erbsland::stream::impl {

StandardTextOutputStreamData::StandardTextOutputStreamData(NativeOutputStreamPtr output) : native{std::move(output)} {
}

void StandardTextOutputStreamData::schedule() {
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
    auto text = queue.front();
    work.submit([self = std::move(self), text = std::move(text)]() -> void { self->performWrite(text); });
}

void StandardTextOutputStreamData::performWrite(const text::String &text) {
    auto failure = std::exception_ptr{};
    try {
        native->writeText(text);
    } catch (const err::RuntimeError &) {
        failure = std::current_exception();
    }
    {
        const auto lock = std::scoped_lock{mutex};
        workInProgress = false;
        if (failure) {
            error = failure;
            streamState.store(StreamState::Failed);
        } else if (!aborted.load()) {
            pendingBytes -= text.length().toSizeT();
            queue.pop_front();
            schedule();
        }
    }
    condition.notify_all();
}

void StandardTextOutputStreamData::scheduleFlush(const bool closeAfterFlush, const uint64_t generation) {
    if (workInProgress || aborted.load()) {
        return;
    }
    workInProgress = true;
    auto self = shared_from_this();
    work.submit([self = std::move(self), closeAfterFlush, generation]() -> void {
        self->performFlush(closeAfterFlush, generation);
    });
}

void StandardTextOutputStreamData::performFlush(const bool closeAfterFlush, const uint64_t generation) {
    auto failure = std::exception_ptr{};
    try {
        native->flush();
    } catch (const err::RuntimeError &) {
        failure = std::current_exception();
    }
    {
        const auto lock = std::scoped_lock{mutex};
        workInProgress = false;
        if (failure) {
            error = failure;
            streamState.store(StreamState::Failed);
        } else if (closeAfterFlush) {
            streamState.store(StreamState::Closed);
        } else {
            completedFlushGeneration = generation;
            schedule();
        }
    }
    condition.notify_all();
}

}
