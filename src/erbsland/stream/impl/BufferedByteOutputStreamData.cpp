// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BufferedByteOutputStreamData.hpp"

#include "IoService.hpp"

#include "../../mem/impl/UnsafeRingBufferAccess.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace erbsland::stream::impl {

BufferedByteOutputStreamData::BufferedByteOutputStreamData(
    NativeByteStreamPtr nativeStream, OutputStreamSettings streamSettings) :
    native{std::move(nativeStream)},
    settings{streamSettings},
    front{settings.bufferCapacity()},
    back{unit::ByteLength{1U}, settings.backBufferLimit()} {
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
    IoService::submitIoWork([self = std::move(self)] { self->performWrite(); });
}

void BufferedByteOutputStreamData::refillFront() {
    constexpr auto cTransferSize = std::size_t{16U * 1024U};
    auto transfer = std::array<mem::Byte, cTransferSize>{};
    while (!front.isFull() && !back.isEmpty()) {
        const auto maximum = std::min(transfer.size(), front.available().toSizeT());
        const auto count = back.read(std::span<mem::Byte>{transfer.data(), maximum});
        static_cast<void>(front.write(std::span<const mem::Byte>{transfer.data(), count.toSizeT()}));
    }
    if (back.isEmpty()) {
        back.shrinkToInitial();
    }
}

void BufferedByteOutputStreamData::performWrite() {
    auto access = std::unique_ptr<mem::impl::UnsafeRingBufferAccess>{};
    auto source = std::span<const mem::Byte>{};
    {
        const auto lock = std::scoped_lock{mutex};
        if (aborted.load()) {
            workInProgress = false;
            condition.notify_all();
            return;
        }
        access = std::make_unique<mem::impl::UnsafeRingBufferAccess>(front);
        source = access->readableSpans()[0];
    }

    auto failure = std::exception_ptr{};
    try {
        native->write(source);
    } catch (...) {
        failure = std::current_exception();
    }

    {
        const auto lock = std::scoped_lock{mutex};
        if (!aborted.load() && !failure) {
            access->consumeRead(unit::ByteLength::fromSizeT(source.size()));
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
    IoService::submitIoWork([self = std::move(self)] { self->performClose(); });
}

void BufferedByteOutputStreamData::performClose() {
    auto failure = std::exception_ptr{};
    try {
        native->flush();
        native->close();
    } catch (...) {
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
