// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Create an owning fixed-size `ByteArray` from a borrowed span.
///
/// `fromSpan()` reports a length mismatch with an empty optional, while
/// `fromSpanOrThrow()` treats the mismatch as an error. Both factories accept
/// only an exact extent and copy the bytes into an independent array.
void copyingFromSpan() {
    auto receivedSamples = el::ByteBuffer{el::Byte{31U}, el::Byte{74U}, el::Byte{121U}, el::Byte{74U}};

    // Use the optional factory when an incoming sample count is expected to vary.
    const auto copiedWave = el::ByteArray<4>::fromSpan(receivedSamples.span());
    const auto wrongExtent = el::ByteArray<5>::fromSpan(receivedSamples.span());
    if (!copiedWave.has_value()) {
        el::io::printLine("The four-sample wave was unavailable."_el);
        return;
    }

    // Use the throwing factory when the format guarantees the exact sample count.
    const auto requiredWave = el::ByteArray<4>::fromSpanOrThrow(receivedSamples.span());

    // Changing the source after the copy does not affect either array.
    receivedSamples.setOrThrow(el::ByteIndex{0U}, el::Byte{255U});

    el::io::printLine("Wave               : Havsvåg"_el);
    el::io::printLine("Copied samples     : "_el, el::ByteFormat::separated(), el::ByteBlock{*copiedWave});
    el::io::printLine("Wrong extent copied: "_el, el::BooleanFormat::yesNo(), wrongExtent.has_value());
    el::io::printLine("Strict copy agrees : "_el, el::BooleanFormat::yesNo(), requiredWave == *copiedWave);
    el::io::printLine("Source first sample: "_el, receivedSamples.get(el::ByteIndex{0U}).toUInt32());
    el::io::printLine("Copy first sample  : "_el, copiedWave->get(el::ByteIndex{0U}).toUInt32());
}

}
