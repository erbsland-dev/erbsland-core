// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Inspect, reserve, clear, and shrink a bounded ring buffer.
///
/// State queries distinguish readable data, currently available storage, and
/// the hard limit for atomic writes. Reserving prepares capacity without adding
/// data; clearing makes data unreadable but keeps the allocation for reuse.
void managingCapacity() {
    auto observations = el::RingBuffer{el::ByteLength{4U}, el::ByteLength{12U}};

    // Reserve enough current capacity for an upcoming atomic observation.
    const auto reserved = observations.reserveAdditional(el::ByteLength{7U});
    const auto reservedCapacity = observations.capacity();
    const auto canWriteTwelve = observations.canWrite(el::ByteLength{12U});
    const auto canWriteThirteen = observations.canWrite(el::ByteLength{13U});

    // Queue data, inspect state, then reuse and finally release excess capacity.
    const auto queued = observations.write(el::ByteArray{el::Byte{2U}, el::Byte{3U}, el::Byte{5U}}.span());
    const auto readable = observations.length();
    const auto available = observations.available();
    observations.clear();
    const auto emptyAfterClear = observations.isEmpty();
    observations.shrinkToInitial();

    el::io::printLine("Monitoraggio      : Laguna corallina"_el);
    el::io::printLine("Reservation       : "_el, el::BooleanFormat::yesNo(), reserved.isSuccessful());
    el::io::printLine("Reserved capacity : "_el, reservedCapacity.toSizeT());
    el::io::printLine("Queued bytes      : "_el, queued.toSizeT());
    el::io::printLine("Readable/available: "_el, readable.toSizeT(), " / "_el, available.toSizeT());
    el::io::printLine(
        "Can write 12 / 13 : "_el,
        el::BooleanFormat::yesNo(),
        canWriteTwelve,
        " / "_el,
        el::BooleanFormat::yesNo(),
        canWriteThirteen);
    el::io::printLine("Empty after clear : "_el, el::BooleanFormat::yesNo(), emptyAfterClear);
    el::io::printLine("Shrunk capacity   : "_el, observations.capacity().toSizeT());
}

}
