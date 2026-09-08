// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <array>
#include <cstddef>
#include <span>

namespace demo {

/// Copy between a byte buffer and standard contiguous storage.
///
/// The `fromSpan()` factories are explicit ownership boundaries for standard
/// byte-like types. The matching vector conversions produce independent data
/// suitable for APIs that do not use Erbsland byte containers.
void convertingBuffers() {
    const auto sensorBytes = std::array<uint8_t, 4>{5U, 8U, 13U, 21U};
    const auto labelBytes = std::array{'O', 'R', 'M', 'A', 'N'};
    const auto flags = std::array{std::byte{0x01U}, std::byte{0x04U}};

    // Copy standard spans into owned buffers.
    const auto sensors = el::ByteBuffer::fromSpan(std::span{sensorBytes});
    const auto label = el::ByteBuffer::fromSpan(std::span{labelBytes});
    const auto featureFlags = el::ByteBuffer::fromSpan(std::span{flags});

    // Copy buffer contents back to the representation expected by another API.
    const auto sensorVector = sensors.toUInt8Vector();
    const auto labelVector = label.toCharVector();

    el::io::printLine("İstasyon          : Orman sınırı"_el);
    el::io::printLine("Sensor bytes      : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(sensors.span()));
    el::io::printLine(
        "Feature flags     : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(featureFlags.span()));
    el::io::printLine("Vector sizes      : "_el, sensorVector.size(), " / "_el, labelVector.size());
}

}
