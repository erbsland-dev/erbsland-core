// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Typed construction builds a `StringPattern` from explicit pattern elements.
///
/// This form skips parsed pattern syntax. Literal text is supplied as UTF-32 through `Text`, and character choices are
/// expressed with `OneChar`, `Range`, and `Set`. `Divider` is the single front/back divider that corresponds to `*` in
/// parsed patterns.
void staticConstruction() {
    using namespace el::pattern;

    static const auto acceptedRecord = el::StringPattern{
        Text{U"probe-"},
        Range{U'A', U'Z'},
        Set{{U'0', U'9'}},
        OneChar{},
        Divider{},
        Text{U";ok"},
    };

    const auto probeLines = el::StringList{{
        "probe-A7b wert=0.42;ok"_el,
        "probe-a7b wert=0.42;ok"_el,
        "probe-A7b wert=0.42;prüfen"_el,
    }};
    const auto lineFormat = el::StringFormat{"{:30}: {}"};
    for (const auto &line : probeLines) {
        el::io::printLine(lineFormat.build(line, acceptedRecord.matches(line)));
    }
}

}
