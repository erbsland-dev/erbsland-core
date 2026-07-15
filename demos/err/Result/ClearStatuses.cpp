// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// A `Result` gives both outcomes meaningful names at the function boundary and at the call site.
/// Unlike a `bool`, the return type communicates that the value reports the outcome of an operation.
[[nodiscard]] auto prepareRobotArm(const el::StringView &armName) noexcept -> el::Result {
    return armName == "Aurora"_el ? el::Result::Success : el::Result::Failure;
}

/// Callers can test the named result without having to remember what `true` or `false` means.
void clearStatuses() {
    if (isSuccessful(prepareRobotArm("Aurora"_el))) {
        el::io::printLine("The Aurora arm is ready."_el);
    }

    if (isFailure(prepareRobotArm("Neblina"_el))) {
        el::io::printLine("The Neblina arm could not be prepared."_el);
    }
}

}
