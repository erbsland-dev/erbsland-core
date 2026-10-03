// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BufferedByteOutputStreamData.hpp"

#include "StreamBufferSizes.hpp"

#include "../../err/RuntimeError.hpp"
#include "../../mem/impl/UnsafeRingBufferAccess.hpp"

#include <algorithm>
#include <cstring>
#include <optional>
#include <utility>

namespace erbsland::stream::impl {

using namespace mem;

BufferedByteOutputStreamData::BufferedByteOutputStreamData(
    NativeByteStreamPtr nativeStream, OutputStreamSettings streamSettings, util::impl::WorkerReservation admission) :
    work{std::move(admission)},
    native{std::move(nativeStream)},
    settings{streamSettings},
    front{streamBufferSizes(settings.buffering()).ioRing},
    back{
        std::min(streamBufferSizes(settings.buffering()).outputRetainedInitial, settings.backBufferLimit()),
        settings.backBufferLimit()} {
    if (native->supportsPositioning()) {
        logicalPosition.store(native->position().toRawValue());
    }
}

void BufferedByteOutputStreamData::scheduleWrite() {
    if (workInProgress || aborted.load()) {
        return;
    }
    if (front.isEmpty()) {
        refillFront();
    }
    if (front.isEmpty()) {
        if (streamState.load() == StreamState::Closing) {
            scheduleClose();
        }
        return;
    }
    workInProgress = true;
    auto self = shared_from_this();
    work.submit([self = std::move(self)]() -> void { self->performWrite(); });
}

void BufferedByteOutputStreamData::refillFront() {
    {
        auto frontAccess = mem::impl::UnsafeRingBufferAccess{front};
        auto backAccess = mem::impl::UnsafeRingBufferAccess{back};
        while (!front.isFull() && !back.isEmpty()) {
            const auto source = backAccess.readableSpans()[0];
            const auto destination = frontAccess.writableSpans()[0];
            const auto count = std::min(source.size(), destination.size());
            std::memcpy(destination.data(), source.data(), count * sizeof(Byte));
            frontAccess.commitWritten(unit::ByteCount::fromSizeT(count));
            backAccess.consumeRead(unit::ByteCount::fromSizeT(count));
        }
    }
    if (back.isEmpty()) {
        back.shrinkToInitial();
    }
}

void BufferedByteOutputStreamData::performWrite() {
    auto access = std::optional<mem::impl::UnsafeRingBufferAccess>{};
    auto source = ConstByteSpan{};
    {
        const auto lock = std::scoped_lock{mutex};
        if (aborted.load()) {
            workInProgress = false;
            condition.notify_all();
            return;
        }
        access.emplace(front);
        source = access->readableSpans()[0];
    }

    auto failure = std::exception_ptr{};
    try {
        native->write(source);
    } catch (const err::RuntimeError &) {
        failure = std::current_exception();
    }

    {
        const auto lock = std::scoped_lock{mutex};
        if (!aborted.load() && !failure) {
            access->consumeRead(unit::ByteCount::fromSizeT(source.size()));
        }
        access.reset();
        workInProgress = false;
        if (failure && !aborted.load()) {
            error = failure;
            streamState.store(StreamState::Failed);
        } else if (!aborted.load()) {
            scheduleWrite();
        }
    }
    condition.notify_all();
}

void BufferedByteOutputStreamData::scheduleClose() {
    if (workInProgress || aborted.load() || streamState.load() != StreamState::Closing) {
        return;
    }
    workInProgress = true;
    auto self = shared_from_this();
    work.submit([self = std::move(self)]() -> void { self->performClose(); });
}

void BufferedByteOutputStreamData::performClose() {
    auto failure = std::exception_ptr{};
    try {
        native->flush();
        native->close();
    } catch (const err::RuntimeError &) {
        failure = std::current_exception();
    }
    {
        const auto lock = std::scoped_lock{mutex};
        workInProgress = false;
        if (failure && !aborted.load()) {
            error = failure;
            streamState.store(StreamState::Failed);
        } else if (!aborted.load()) {
            streamState.store(StreamState::Closed);
        }
    }
    condition.notify_all();
}

}
