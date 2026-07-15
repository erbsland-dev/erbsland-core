// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BufferedByteInputStreamData.hpp"

#include "IoService.hpp"

#include "../../mem/impl/UnsafeRingBufferAccess.hpp"

#include <span>
#include <utility>

namespace erbsland::stream::impl {

BufferedByteInputStreamData::BufferedByteInputStreamData(
    NativeByteStreamPtr nativeStream, InputStreamSettings streamSettings) :
    native{std::move(nativeStream)},
    settings{streamSettings},
    front{settings.bufferCapacity()},
    back{settings.bufferCapacity()} {
    if (native->supportsPositioning()) {
        logicalPosition.store(native->position().toRawValue());
    }
}

void BufferedByteInputStreamData::scheduleRead() {
    if (readInProgress || positioning || finished || aborted.load() || streamState.load() != StreamState::Open ||
        !back.isEmpty()) {
        return;
    }
    readInProgress = true;
    auto self = shared_from_this();
    IoService::submitIoWork([self = std::move(self)] { self->performRead(); });
}

void BufferedByteInputStreamData::performRead() {
    auto access = std::unique_ptr<mem::impl::UnsafeRingBufferAccess>{};
    auto destination = std::span<mem::Byte>{};
    {
        const auto lock = std::scoped_lock{mutex};
        if (aborted.load()) {
            readInProgress = false;
            condition.notify_all();
            return;
        }
        access = std::make_unique<mem::impl::UnsafeRingBufferAccess>(back);
        const auto spans = access->writableSpans();
        destination = spans[0];
    }

    auto readLength = unit::ByteLength::zero();
    auto failure = std::exception_ptr{};
    try {
        readLength = native->read(destination);
    } catch (...) {
        failure = std::current_exception();
    }

    {
        const auto lock = std::scoped_lock{mutex};
        if (!aborted.load() && !failure) {
            access->commitWritten(readLength);
            finished = readLength.isZero();
        }
        access.reset();
        readInProgress = false;
        if (failure && !aborted.load()) {
            error = failure;
            streamState.store(StreamState::Failed);
        } else if (!aborted.load() && front.isEmpty() && !back.isEmpty()) {
            front.swap(back);
            scheduleRead();
        }
    }
    condition.notify_all();
}

}
