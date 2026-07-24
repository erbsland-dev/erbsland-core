// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BufferedByteInputStreamData.hpp"

#include "IoService.hpp"
#include "StreamBufferSizes.hpp"

#include "../../mem/impl/UnsafeByteBlockBuffer.hpp"
#include "../../mem/impl/UnsafeRingBufferAccess.hpp"

#include <optional>
#include <span>
#include <utility>

namespace erbsland::stream::impl {

BufferedByteInputStreamData::BufferedByteInputStreamData(
    NativeByteStreamPtr nativeStream, InputStreamSettings streamSettings, const bool useRuntimeSensitive) :
    native{std::move(nativeStream)},
    settings{streamSettings},
    front{streamBufferSizes(settings.buffering()).ioRing},
    back{streamBufferSizes(settings.buffering()).ioRing},
    runtimeSensitive{useRuntimeSensitive} {
    front.setSensitive(settings.isSensitive());
    back.setSensitive(settings.isSensitive());
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
    IoService::submitIoWork([self = std::move(self)]() -> void { self->performRead(); });
}

void BufferedByteInputStreamData::performRead() {
    auto access = std::optional<mem::impl::UnsafeRingBufferAccess>{};
    auto transfer = std::optional<mem::impl::UnsafeByteBlockBuffer>{};
    auto destination = mem::ByteSpan{};
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
            transfer.emplace(back.available(), true);
            destination = transfer->data();
        } else {
            access.emplace(back);
            destination = access->writableSpans()[0];
        }
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
        if (!aborted.load() && !failure && epoch == sensitivityEpoch.load()) {
            if (runtimeSensitive) {
                const auto privateBytes = transfer->take(readLength);
                static_cast<void>(back.write(privateBytes.span()));
            } else {
                access->commitWritten(readLength);
            }
            finished = readLength.isZero();
        }
        access.reset();
        readInProgress = false;
        if (failure && !aborted.load()) {
            error = failure;
            streamState.store(StreamState::Failed);
        } else if (!aborted.load()) {
            if (front.isEmpty() && !back.isEmpty()) {
                front.swap(back);
            }
            scheduleRead();
        }
    }
    condition.notify_all();
}

}
