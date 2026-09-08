// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Reuse a byte buffer while controlling its visible size and allocation.
///
/// `resize()` changes the byte sequence and zero-fills growth. `reserve()` only
/// prepares storage, while `clear()` preserves that storage for the next cycle
/// and `shrinkToFit()` releases capacity that is no longer useful.
void managingStorage() {
    auto samples = el::ByteBuffer{el::Byte{21U}, el::Byte{34U}};

    // Prepare room for a larger survey without adding visible bytes.
    samples.reserve(el::ByteLength{16U});
    const auto reservedCapacity = samples.capacity();

    // Extend the visible data; all new sample positions start at zero.
    samples.resize(el::ByteLength{5U});
    const auto grownSamples = el::ByteBlock::fromSpan(samples.span());

    // Reuse the allocation for a new survey, then trim it to the final result.
    samples.clear().append(el::Byte{8U}).append(el::Byte{13U});
    const auto capacityAfterClear = samples.capacity();
    samples.shrinkToFit();

    el::io::printLine("Gözlem            : Meşe yenilenmesi"_el);
    el::io::printLine("Reserved capacity : "_el, reservedCapacity.toSizeT());
    el::io::printLine("Grown samples     : "_el, el::ByteFormat::separated(), grownSamples);
    el::io::printLine("Length / end index: "_el, samples.length().toSizeT(), " / "_el, samples.endIndex().toSizeT());
    el::io::printLine("Capacity after clear: "_el, capacityAfterClear.toSizeT());
    el::io::printLine("Final capacity    : "_el, samples.capacity().toSizeT());
}

}
