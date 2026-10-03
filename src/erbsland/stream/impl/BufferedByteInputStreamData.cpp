// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BufferedByteInputStreamData.hpp"

#include "StreamBufferSizes.hpp"

#include "../../err/RuntimeError.hpp"
#include "../../mem/impl/UnsafeByteBlockBuffer.hpp"
#include "../../mem/impl/UnsafeRingBufferAccess.hpp"

#include <memory>
#include <optional>
#include <span>
#include <utility>

namespace erbsland::stream::impl {

using namespace mem;

BufferedByteInputStreamData::BufferedByteInputStreamData(
    NativeByteStreamPtr nativeStream, InputStreamSettings streamSettings, const bool useRuntimeSensitive) :
    native{std::move(nativeStream)}, settings{streamSettings}, runtimeSensitive{useRuntimeSensitive} {
    if (native->supportsPositioning()) {
        logicalPosition.store(native->position().toRawValue());
    }
}

void BufferedByteInputStreamData::scheduleRead() {
    if (readInProgress || positioning || finished || aborted.load() || streamState.load() != StreamState::Open ||
        !backIsEmpty()) {
        return;
    }
    if (!back) {
        back = std::make_unique<RingBuffer>(streamBufferSizes(settings.buffering()).ioRing);
        back->setSensitive(settings.isSensitive());
    }
    readInProgress = true;
    auto self = shared_from_this();
    work.submit([self = std::move(self)]() -> void { self->performRead(); });
}

void BufferedByteInputStreamData::performRead() {
    auto access = std::optional<mem::impl::UnsafeRingBufferAccess>{};
    auto transfer = std::optional<mem::impl::UnsafeByteBlockBuffer>{};
    auto destination = ByteSpan{};
    auto epoch = uint64_t{};
    {
        const auto lock = std::scoped_lock{mutex};
        if (aborted.load()) {
            readInProgress = false;
            condition.notify_all();
            return;
        }
        epoch = sensitivityEpoch.load();
        if (runtimeSensitive) {
            transfer.emplace(back->available(), true);
            destination = transfer->data();
        } else {
            access.emplace(*back);
            destination = access->writableSpans()[0];
        }
    }

    auto readLength = unit::ByteCount::zero();
    auto reachedEnd = false;
    auto failure = std::exception_ptr{};
    try {
        // Regular files may complete short reads before EOF (including files that change after opening).
        // Confirm EOF in this same worker operation, but never wait for another chunk from a pipe or device.
        const auto fillBuffer = native->initialFileSize().isFinite();
        do {
            const auto count = native->read(destination.subspan(readLength.toSizeT()));
            if (count.isZero()) {
                reachedEnd = true;
                break;
            }
            readLength += count;
        } while (fillBuffer && readLength.toSizeT() < destination.size() && !aborted.load());
    } catch (const err::RuntimeError &) {
        failure = std::current_exception();
    }

    {
        const auto lock = std::scoped_lock{mutex};
        if (!aborted.load() && !failure && epoch == sensitivityEpoch.load()) {
            if (runtimeSensitive) {
                const auto privateBytes = transfer->take(readLength);
                if (back->write(privateBytes.span()) != readLength) {
                    std::terminate();
                }
            } else {
                access->commitWritten(readLength);
            }
            finished = reachedEnd;
        }
        access.reset();
        readInProgress = false;
        if (failure && !aborted.load()) {
            error = failure;
            streamState.store(StreamState::Failed);
        } else if (!aborted.load()) {
            if (frontIsEmpty() && !backIsEmpty()) {
                front.swap(back);
            }
            scheduleRead();
        }
    }
    condition.notify_all();
}

}
