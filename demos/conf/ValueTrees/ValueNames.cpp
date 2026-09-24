// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>

namespace demo {

/// Inspect a value's local name and absolute name path.
///
/// `name()` identifies a value among its siblings. `namePath()` describes the route from the document root, including
/// indexes introduced by value lists and section lists.
void valueNames() {
    const auto configuration = "*[patch.oscillators]*\n"
                               "waveform: \"square\"\n"
                               "*[patch.oscillators]*\n"
                               "waveform: \"triangle\"\n"_el;
    const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
    const auto waveform = document->valueOrThrow("patch.oscillators[1].waveform"_el);

    el::io::printLine("Local name: "_el, waveform->name().toPathText());
    el::io::printLine("Absolute path: "_el, waveform->namePath().toText());
}

}
