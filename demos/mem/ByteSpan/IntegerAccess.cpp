// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Read and write integer fields in a borrowed byte span.
///
/// The integer-access functions handle signed and unsigned native integers in
/// little- or big-endian byte order. Their checked variants either report an
/// invalid range without changing data, return a chosen fallback value, or
/// throw when an invalid layout is a programming error.
void integerAccess() {
    auto record = std::array<el::Byte, 12>{};
    auto writable = el::ByteSpan{record};

    // Encode fields in the byte order defined by the observation format.
    const auto wavelengthStored = el::setInteger(writable, el::ByteIndex{0U}, uint16_t{656U}, el::Endianness::Big);
    el::setIntegerOrThrow(writable, el::ByteIndex{2U}, int16_t{-18}, el::Endianness::Little);
    el::setIntegerOrThrow(writable, el::ByteIndex{4U}, uint32_t{1'350'000U}, el::Endianness::Big);

    // Decode fields without copying the record into an intermediate structure.
    const auto readOnly = el::ConstByteSpan{record};
    const auto wavelength = el::getIntegerOrThrow<uint16_t>(readOnly, el::ByteIndex{0U}, el::Endianness::Big);
    auto temperature = int16_t{};
    const auto temperatureRead = el::getIntegerInto(readOnly, temperature, el::ByteIndex{2U}, el::Endianness::Little);
    const auto exposure = el::getInteger<uint32_t>(readOnly, el::ByteIndex{4U}, el::Endianness::Big);
    const auto unavailable =
        el::getInteger<uint32_t>(readOnly, el::ByteIndex{10U}, el::Endianness::Big, uint32_t{999U});

    el::io::printLine("Observation        : Νεφέλωμα του Ωρίωνα"_el);
    el::io::printLine("Wavelength stored  : "_el, el::BooleanFormat::yesNo(), wavelengthStored);
    el::io::printLine("Temperature read   : "_el, el::BooleanFormat::yesNo(), temperatureRead);
    el::io::printLine("Wavelength (nm)    : "_el, wavelength);
    el::io::printLine("Temperature (C)    : "_el, temperature);
    el::io::printLine("Exposure (us)      : "_el, exposure);
    el::io::printLine("Missing fallback   : "_el, unavailable);
}

}
