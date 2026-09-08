// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <cstdint>

namespace demo {

/// Queue complete integer fields in a byte ring buffer.
///
/// `ByteRingBuffer` applies one configured byte order to fixed-width integers.
/// Writes are atomic at the hard capacity limit, and reads return an empty
/// optional without consuming data when a complete value is unavailable.
void readingAndWritingIntegers() {
    auto measurements = el::ByteRingBuffer{el::ByteLength{4U}, el::ByteLength{8U}};
    measurements.setEndianness(el::Endianness::Big);

    // Queue two complete fields in their stream order.
    const auto temperatureWritten = measurements.writeInteger<int16_t>(245);
    const auto colonyWritten = measurements.writeInteger<uint32_t>(120000U);

    // Read the same field types in FIFO order.
    const auto temperature = measurements.readInteger<int16_t>();
    const auto colonySize = measurements.readInteger<uint32_t>();
    const auto missingValue = measurements.readInteger<uint16_t>();

    el::io::printLine("Rilevamento       : Giardino di corallo"_el);
    el::io::printLine(
        "Fields written    : "_el,
        el::BooleanFormat::yesNo(),
        temperatureWritten.isSuccessful() && colonyWritten.isSuccessful());
    el::io::printLine("Temperature       : "_el, temperature.value_or(0));
    el::io::printLine("Colony size       : "_el, colonySize.value_or(0U));
    el::io::printLine("Extra field       : "_el, el::BooleanFormat::yesNo(), missingValue.has_value());
}

}
