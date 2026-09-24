// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>

namespace demo {

/// Turn a validated document into an application-owned settings object.
///
/// Keeping the parsed document only until its values have been checked and copied makes ownership clear. It also
/// shortens the time in which the parser's source text and sensitive configuration values remain reachable.
void loadingStrategies() {
    struct PatchSettings {
        el::String name;
        el::conf::Integer voices;
        bool stereo;
    };

    const auto configuration = "[patch]\n"
                               "name: \"Ay Işığı\"\n"
                               "voices: 6\n"
                               "stereo: yes\n"_el;

    // Load and validate all required values before changing application state.
    auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
    auto settings = PatchSettings{
        .name = document->getTextOrThrow("patch.name"_el),
        .voices = document->getIntegerOrThrow("patch.voices"_el),
        .stereo = document->getBoolean("patch.stereo"_el, false),
    };
    if (settings.voices < 1 || settings.voices > 32) {
        throw el::RuntimeError{"The patch must use between 1 and 32 voices."_el};
    }

    // The application no longer needs the document after interpretation.
    document.reset();
    el::io::printLine(settings.name, ": "_el, settings.voices, " voices"_el);
}

}
