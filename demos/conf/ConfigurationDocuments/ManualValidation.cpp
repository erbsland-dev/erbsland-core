// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>

namespace demo {

/// Validate a small document directly while reading its values.
///
/// Throwing accessors are a compact way to require a value and its type. When several types are acceptable, first
/// retrieve the value with `valueOrThrow()` and inspect it with the `is...()` methods.
void manualValidation(const el::Path &configurationPath) {
    const auto document = el::conf::Parser{}.parseFileOrThrow(configurationPath);

    // These calls require both values and verify their native types.
    const auto name = document->getTextOrThrow("patch.name"_el);
    const auto voices = document->getIntegerOrThrow("patch.voices"_el);
    if (voices < 1 || voices > 32) {
        throw el::RuntimeError{"The patch must use between 1 and 32 voices."_el};
    }

    // Inspect the value when the application accepts more than one representation.
    const auto waveform = document->valueOrThrow("patch.oscillator.waveform"_el);
    if (!waveform->isText() && !waveform->isInteger()) {
        throw el::RuntimeError{"The waveform must be text or an integer identifier."_el};
    }

    el::io::printLine("Validated patch: "_el, name);
}

}
