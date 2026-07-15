// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Random value helpers cover integers, floating-point values, booleans, bytes,
/// and strings.
///
/// Integer ranges are inclusive and can be passed either as two bounds or as an
/// `IntegerRange`. Repeated draws, byte blocks, and random strings are built from
/// the same shared application generator, so application code does not need to
/// seed or manage its own engine for ordinary random values.
void randomValues() {
    auto &random = el::application().random();

    // Draw individual integer values and reuse domain ranges.
    const auto chamber = random.getUInt32(1U, 6U);
    const auto chargeRange = el::IntegerRange<int>{12, 30};
    const auto charge = random.selectInteger(chargeRange);
    const auto pulseLevels = random.buildIntegerList(el::ElementCount{5U}, 1, 4);

    el::io::printLine("Artifact chamber   : "_el, chamber);
    el::io::printLine("Runic charge       : "_el, charge);
    el::io::printLine(
        "Pulses             : "_el, pulseLevels.count().toSizeT(), " draws, first="_el, pulseLevels.first());

    // Draw non-integer values from the same random source.
    const auto resonance = random.getDouble(0.25, 0.95);
    const auto isAwake = random.getBool();
    const auto resonanceText = el::StringFormat{"Resonance          : {:.2f}"_el}.build(resonance);
    el::io::printLine(resonanceText);
    el::io::printLine("Guardian awake     : "_el, el::BooleanFormat::yesNo(), isAwake);

    // Build byte and string values when the caller needs generated data.
    const auto bytes = random.buildByteBlock(el::ByteLength{4U});
    const auto alphabet = el::CharSet::fromPattern("A-Z0-9"_el);
    const auto label = random.buildString(el::CpLength{10U}, alphabet);
    el::io::printLine("First byte         : "_el, static_cast<unsigned>(bytes.get(el::ByteIndex{0U}).toUInt8()));
    el::io::printLine("Generated label    : "_el, label);
}

}
