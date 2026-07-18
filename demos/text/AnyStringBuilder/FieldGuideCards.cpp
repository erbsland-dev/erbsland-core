// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

struct Observation {
    el::String symbol;
    el::String species;
    el::String place;
    el::String count;
    el::String note;
};

void appendFieldGuideCard(el::AnyStringBuilder &builder, const Observation &observation);

/// `AnyStringBuilder` efficiently builds strings in memory.
/// It can be reused, moved out with `takeString()`, and target UTF-8, UTF-16,
/// or UTF-32 while generic helper functions keep the same signature.
void fieldGuideCards() {
    const auto fern = Observation{
        .symbol = "🌿"_el,
        .species = "Farn im Moos"_el,
        .place = "Bachufer · 水辺"_el,
        .count = "7 fronds"_el,
        .note = "New leaves curl like tiny green clocks."_el,
    };
    const auto oak = Observation{
        .symbol = "🌳"_el,
        .species = "chêne ancien"_el,
        .place = "Forêt claire · północ"_el,
        .count = "3 seedlings"_el,
        .note = "Acorns found beside warm limestone."_el,
    };
    const auto cypress = Observation{
        .symbol = "🌲"_el,
        .species = "κυπαρίσσι"_el,
        .place = "Sun trail · camino del sol"_el,
        .count = "12 cones"_el,
        .note = "Resin scent after noon rain."_el,
    };

    // Build a UTF-8 field guide page for display in the terminal.
    auto builder = el::AnyStringBuilder{};

    appendFieldGuideCard(builder, fern);
    el::io::printLine("First card preview:"_el);
    el::io::print(builder.toString());
    el::io::printLine("Length after first card: "_el, builder.length());

    appendFieldGuideCard(builder, oak);
    el::io::printLine("Length after second card: "_el, builder.length());

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
    builder.append("│ Place : "_el).append(observation.place).append(U'\n');
    builder.append("│ Count : "_el).append(observation.count).append(U'\n');
    builder.append("│ Note  : "_el).append(observation.note).append(U'\n');
    builder.append(U'╰').append(U'─', el::CpLength{58}).append("╯\n"_el);
}

}
