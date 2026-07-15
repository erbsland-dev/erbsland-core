// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

void processAnyString(const el::AnyStringView &str);

/// `AnyStringView` accepts any string type — UTF-8, UTF-16, or UTF-32 — through a
/// single unified interface. Use it to write functions that receive strings regardless
/// of their underlying encoding, then inspect the kind, length, or convert to the
/// format you need for further processing.
void acceptAny() {
    // Prepare sound-wave labels in three different encodings.
    const auto u8Label = el::U8StringView{"Vlnová frekvence 🌊"_el};
    const auto u16Label = el::U16StringView{u"Hmotnostní spektrum 🎵"_el};
    const auto u32Label = el::U32StringView{U"Amplituda vlnění 🎶"_el};

    // An empty AnyStringView carries no kind information.
    processAnyString({});
    processAnyString(u8Label);
    processAnyString(u16Label);
    processAnyString(u32Label);
}

void processAnyString(const el::AnyStringView &str) {
    el::io::printLine("Signal analysis:"_el);

    if (str.kind().has_value()) {
        el::io::printLine("  Type: "_el, el::toString(str.kind().value()));
    } else {
        el::io::printLine("  Type: (empty)"_el);
    }

    el::io::printLine("  Character length: "_el, str.characterLength());
    el::io::printLine("  Is empty: "_el, str.isEmpty() ? "yes" : "no");

    auto u8Str = str.toU8String();
    u8Str.replaceAll("vlnění"_el, "vlny"_el);
    el::io::printLine("  Result: "_el, u8Str);
    el::io::printLine();
}

}
