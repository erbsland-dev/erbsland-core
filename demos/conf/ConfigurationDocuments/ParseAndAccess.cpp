// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>

namespace demo {

/// Parse an ELCL document and access its typed values.
///
/// `Parser::parseTextOrThrow()` is convenient for embedded text. For files, use `parseFileOrThrow()`; for a custom
/// source, create a `Source` and pass it to `parseOrThrow()`. The resulting document provides typed `get...()` methods
/// that resolve complete name paths without manual tree traversal.
void parseAndAccess() {
    const auto configuration = "[patch]\n"
                               "name: \"Gece Göğü\"\n"
                               "voices: 8\n"
                               "stereo: yes\n"_el;

    // Parse the text and let a configuration error propagate to the application.
    auto parser = el::conf::Parser{};
    const auto document = parser.parseTextOrThrow(configuration);

    // Required values use throwing accessors; optional values provide a default.
    const auto name = document->getTextOrThrow("patch.name"_el);
    const auto voices = document->getIntegerOrThrow("patch.voices"_el);
    const auto stereo = document->getBoolean("patch.stereo"_el, false);

    el::io::printLine("Patch: "_el, name);
    el::io::printLine("Voices: "_el, voices);
    el::io::printLine("Stereo: "_el, el::BooleanFormat::yesNo(), stereo);
}

}
