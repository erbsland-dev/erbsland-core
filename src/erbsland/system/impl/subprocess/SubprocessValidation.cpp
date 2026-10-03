// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SubprocessValidation.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../text/CharSet.hpp"
#include "../../../text/Literals.hpp"

#include <limits>

namespace erbsland::system::impl {

using namespace err;
using namespace text::literals;
void validateSubprocess(
    const path::Path &executable,
    const text::StringList &arguments,
    const SubprocessOptions &options,
    const bool detached) {
    if (executable.isEmpty() || !executable.isValid()) {
        throw ParameterFailure{"The subprocess executable path must be non-empty and valid."_el, "executable"_el};
    }
    for (const auto &argument : arguments) {
        if (!argument.isEncodingValid() || argument.containsOneOf(text::CharSet{U'\0'})) {
            throw ParameterFailure{
                "Subprocess arguments must be valid UTF-8 without null characters."_el, "arguments"_el};
        }
    }
    if (options.workingDirectory().has_value() &&
        (options.workingDirectory()->isEmpty() || !options.workingDirectory()->isValid())) {
        throw ParameterFailure{
            "The subprocess working directory must be non-empty and valid."_el, "workingDirectory"_el};
    }
    if (options.maximumOutputChunkSize().isInfinite() || options.maximumOutputChunkSize() < unit::ByteCount{256U} ||
        options.maximumOutputChunkSize().toRawValue() > std::numeric_limits<std::size_t>::max()) {
        throw ParameterFailure{
            "Output chunk size must be finite, representable, and at least 256 bytes."_el, "maximumOutputChunkSize"_el};
    }
    if (options.captureLimit().isZero() || options.captureLimit() > SubprocessOptions::cMaximumCaptureLimit) {
        throw ParameterFailure{
            "The subprocess capture limit must be positive and within the supported maximum."_el, "captureLimit"_el};
    }
    if (detached &&
        (options.standardOutputMode() == SubprocessOutputMode::Capture ||
            options.standardErrorMode() == SubprocessOutputMode::Capture)) {
        throw ParameterFailure{"Detached subprocesses cannot capture output."_el, "options"_el};
    }
}

}
