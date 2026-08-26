// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

struct Observation {
    el::String symbol;
    el::String species;
    el::String place;
    int count;
    double moisture;
    el::ByteBlock sampleId;
    el::String note;
};

void appendFieldGuideCard(el::AnyStringBuilder &builder, const Observation &observation);

/// `AnyStringBuilder` efficiently builds strings in memory.
/// It appends integers, floating-point values, byte blocks, and differently
/// encoded text directly. Its cached `length()` avoids rescanning the built
/// text to count decoded code points.
void fieldGuideCards() {
    const auto fern = Observation{
        .symbol = "🌿"_el,
        .species = "Farn im Moos"_el,
        .place = "Bachufer · 水辺"_el,
        .count = 7,
        .moisture = 62.5,
        .sampleId = el::ByteBlock{el::Byte{0x0fU}, el::Byte{0xa7U}},
        .note = "New leaves curl like tiny green clocks."_el,
    };
    const auto oak = Observation{
        .symbol = "🌳"_el,
        .species = "chêne ancien"_el,
        .place = "Forêt claire · północ"_el,
        .count = 3,
        .moisture = 48.25,
        .sampleId = el::ByteBlock{el::Byte{0x10U}, el::Byte{0x3cU}},
        .note = "Acorns found beside warm limestone."_el,
    };
    const auto cypress = Observation{
        .symbol = "🌲"_el,
        .species = "κυπαρίσσι"_el,
        .place = "Sun trail · camino del sol"_el,
        .count = 12,
        .moisture = 31.75,
        .sampleId = el::ByteBlock{el::Byte{0x12U}, el::Byte{0xceU}},
        .note = "Resin scent after noon rain."_el,
    };

    // Build a UTF-8 field guide page for display in the terminal. The helper
    // also receives UTF-16 input and converts it directly into the target.
    auto builder = el::AnyStringBuilder{};

    appendFieldGuideCard(builder, fern);
    el::io::printLine("First card preview:"_el);
    el::io::print(builder.toString());
    el::io::printLine("Cached code-point length after first card: "_el, builder.length());

    appendFieldGuideCard(builder, oak);
    el::io::printLine("Cached code-point length after second card: "_el, builder.length());

    // Move the completed page out, then continue with the same builder.
    auto guidePage = builder.takeString();
    el::io::printLine();
    el::io::printLine("Taken field guide draft:"_el);
    el::io::print(guidePage);
    el::io::printLine("After take: "_el, builder.length());

    appendFieldGuideCard(builder, cypress);
    el::io::printLine();
    el::io::printLine("Reused builder:"_el);
    el::io::print(builder.toString());

    // Use the same helper with builders that create UTF-16 and UTF-32 strings.
    auto u16Builder = el::AnyStringBuilder{el::StringKind::U16};
    auto u32Builder = el::AnyStringBuilder{el::StringKind::U32};
    appendFieldGuideCard(u16Builder, cypress);
    appendFieldGuideCard(u32Builder, cypress);

    const auto u16Export = u16Builder.toU16String();
    const auto u32Export = u32Builder.toU32String();

    el::io::printLine();
    el::io::printLine("UTF-16 export:"_el);
    el::io::printLine(u16Export);
    el::io::printLine("UTF-32 export:"_el);
    el::io::printLine(u32Export);
}

void appendFieldGuideCard(el::AnyStringBuilder &builder, const Observation &observation) {
    if (!builder.isEmpty()) {
        builder.append(U'\n');
    }

    builder.append("╭─ "_el)
        .append(observation.symbol)
        .append(U' ')
        .append(observation.species)
        .append(U' ')
        .append(U'─', el::CpLength{53} - observation.species.characterLength() - observation.symbol.characterLength())
        .append("╮\n"_el);
    builder.append(u"│ Place : "_el).append(observation.place).append(U'\n');
    builder.append("│ Count : "_el).appendInteger(observation.count).append(U'\n');
    builder.append("│ Moist.: "_el).appendFloat(observation.moisture).append(" %\n"_el);
    builder.append("│ Sample: "_el).appendByteBlock(observation.sampleId, el::ByteFormat::compact()).append(U'\n');
    builder.append("│ Note  : "_el).append(observation.note).append(U'\n');
    builder.append(U'╰').append(U'─', el::CpLength{58}).append("╯\n"_el);
}

}
