// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>

namespace demo {

/// Address values with strings, names, name paths, and indexes.
///
/// Value-tree methods accept `NamePathLike`, so simple code can pass a Core string while code that reuses or constructs
/// paths can pass `Name` or `NamePath`. Lists additionally accept an index directly.
void namePaths() {
    const auto configuration = "[patch]\n"
                               "name: \"Gökyüzü\"\n"
                               "*[patch.oscillators]*\n"
                               "waveform: \"sine\"\n"_el;
    const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);

    // A string is the concise choice for a complete path.
    const auto patchName = document->getTextOrThrow("patch.name"_el);

    // Name and NamePath objects are useful when paths are assembled or reused.
    const auto patch = document->valueOrThrow(el::conf::Name::createRegular("patch"_el));
    const auto oscillatorPath = el::conf::NamePath::fromText("oscillators"_el);
    const auto oscillators = patch->valueOrThrow(oscillatorPath);

    // Lists accept a numeric index as a path-like value.
    const auto firstOscillator = oscillators->valueOrThrow(std::size_t{0});
    el::io::printLine(patchName, " waveform: "_el, firstOscillator->getTextOrThrow("waveform"_el));
}

}
