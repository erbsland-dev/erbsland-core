// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

[[nodiscard]] auto enableSafetyScanner(const bool areaIsClear) noexcept -> el::Result {
    return areaIsClear ? el::Result::Success : el::Result::Failure;
}

[[nodiscard]] auto loadMissionOrThrow(const el::StringView &missionName) -> el::StringView {
    if (missionName != "Explorar Marte"_el) {
        throw el::RuntimeError{"The mission plan was not found."_el};
    }
    return missionName;
}

/// Use `Result` for an expected local outcome that the immediate caller can handle.
/// Throw an exception when a function cannot produce its promised value and a wider boundary should handle the error.
void resultOrException() {
    // A blocked safety scanner is an expected state with an immediate response.
    if (enableSafetyScanner(false).isFailure()) {
        el::io::printLine("Move away from the robot before starting."_el);
    }

    // A missing mission prevents this function from returning the promised mission name.
    try {
        el::io::printLine("Mission: "_el, loadMissionOrThrow("Mapear Europa"_el));
    } catch (const el::RuntimeError &error) {
        el::io::printLine("Mission failure: "_el, error.reason());
    }
}

}
