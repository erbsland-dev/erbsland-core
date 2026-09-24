// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>

namespace demo {

/// Navigate down to child values and back to their parents.
///
/// Use `hasValue()` for an inexpensive existence test, `value()` for optional branches, and `valueOrThrow()` for
/// required branches. Every non-root value keeps a parent link, making it possible to return to its container.
void navigatingTree() {
    const auto configuration = "[patch]\n"
                               "name: \"Sessiz Kıyı\"\n"
                               "[patch.filter]\n"
                               "mode: \"low-pass\"\n"
                               "cutoff: 2400\n"_el;
    const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
    const auto patch = document->valueOrThrow("patch"_el);

    if (patch->hasValue("filter"_el)) {
        const auto filter = patch->value("filter"_el);
        const auto cutoff = filter->valueOrThrow("cutoff"_el);
        el::io::printLine("Cutoff: "_el, cutoff->asIntegerOrThrow(), " Hz"_el);
        el::io::printLine("Container: "_el, cutoff->parent()->namePath().toText());
    }

    el::io::printLine("Document has parent: "_el, el::BooleanFormat::yesNo(), document->hasParent());
}

}
