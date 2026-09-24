// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>

namespace demo {

/// Iterate over section-list entries in document order.
///
/// Every container supports range-based iteration. When random access is more convenient, combine `size()` with
/// `valueOrThrow(index)`; both approaches return the same child values.
void iteratingLists() {
    const auto configuration = "*[patch.oscillators]*\n"
                               "waveform: \"sine\"\n"
                               "octave: 0\n"
                               "*[patch.oscillators]*\n"
                               "waveform: \"triangle\"\n"
                               "octave: 1\n"_el;
    const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
    const auto oscillators = document->valueOrThrow("patch.oscillators"_el);

    // Range-based iteration is the clearest choice when every entry is processed.
    for (const auto &oscillator : *oscillators) {
        el::io::printLine(
            oscillator->getTextOrThrow("waveform"_el), " at octave "_el, oscillator->getIntegerOrThrow("octave"_el));
    }

    // Indexed access is available when position matters.
    const auto last = oscillators->valueOrThrow(oscillators->size() - 1);
    el::io::printLine("Last oscillator: "_el, last->getTextOrThrow("waveform"_el));
}

}
