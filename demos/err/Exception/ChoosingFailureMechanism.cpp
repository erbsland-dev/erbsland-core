// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

auto prepareInstrument(const el::StringView &instrument) noexcept -> el::Result;
void prepareInstrumentOrThrow(const el::StringView &instrument);

/// Erbsland Core offers three complementary ways to report failure.
/// Use `Result` for a small status returned to the immediate caller, `ExitCode` at a process boundary, and an
/// exception when a function cannot produce its promised value and callers may add diagnostic context.
void choosingFailureMechanism() {
    // A result makes an expected local outcome part of normal control flow.
    if (isSuccessful(prepareInstrument("尺八"_el))) {
        el::io::printLine("The shakuhachi is ready."_el);
    }

    // The throwing variant lets a more distant caller handle the failure.
    try {
        prepareInstrumentOrThrow("篳篥"_el);
    } catch (const el::RuntimeError &error) {
        el::io::printLine("Exception: "_el, error.reason());
    }

    // ApplicationError transports the final process exit code to Application::run().
    throw el::ApplicationError{"The concert preparation could not be completed."_el, el::ExitCode{3}};
}

auto prepareInstrument(const el::StringView &instrument) noexcept -> el::Result {
    return instrument == "尺八"_el ? el::Result::Success : el::Result::Failure;
}

void prepareInstrumentOrThrow(const el::StringView &instrument) {
    if (isFailure(prepareInstrument(instrument))) {
        throw el::RuntimeError{el::String::fromJoined({"The instrument could not be prepared: "_el, instrument})};
    }
}

}
