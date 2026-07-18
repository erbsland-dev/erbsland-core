// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <compare>

namespace demo {

auto hashToString(std::size_t hash) -> el::String {
    static auto format = el::StringFormat{"0x{:016x}"};
    return format.build(hash);
}

/// This demo shows how to make use of `String`s hash functions.
void hashing() {
    // Here we create a set of different read-only strings.
    // For real code use the literals directly, like `const auto x = "abc"_el;`
    const auto titlecase = el::String{"Fichte"_el};
    const auto lowercase = el::String{"fichte"_el};
    const auto greek = el::String{"Σκιά"_el};
    const auto greekLower = el::String{"σκιά"_el};

    el::io::printLine("titlecase ............: "_el, titlecase);
    el::io::printLine("lowercase ............: "_el, lowercase);
    el::io::printLine("greek ................: "_el, greek);
    el::io::printLine("greekLower ...........: "_el, greekLower);
    el::io::printLine();

    // String hashes let you use read-only strings in sets and unordered maps.
    el::io::printLine("\nRegular Hash Values:"_el);
    auto hash = titlecase.toHash();
    el::io::printLine("  titlecase.toHash()     → "_el, hashToString(hash));
    hash = lowercase.toHash();
    el::io::printLine("  lowercase.toHash()     → "_el, hashToString(hash));
    hash = greek.toHash();
    el::io::printLine("  greek.toHash()         → "_el, hashToString(hash));
    hash = greekLower.toHash();
    el::io::printLine("  greekLower.toHash()    → "_el, hashToString(hash));

    el::io::printLine("\nCase-Insensitive Hash Values:"_el);
    hash = titlecase.toHashCI();
    el::io::printLine("  titlecase.toHashCI()   → "_el, hashToString(hash));
    hash = lowercase.toHashCI();
    el::io::printLine("  lowercase.toHashCI()   → "_el, hashToString(hash));
    hash = greek.toHashCI();
    el::io::printLine("  greek.toHashCI()       → "_el, hashToString(hash));
    hash = greekLower.toHashCI();
    el::io::printLine("  greekLower.toHashCI()  → "_el, hashToString(hash));
}

}
