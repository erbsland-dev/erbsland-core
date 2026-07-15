// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// The neutral exception types describe common failure categories without tying them to a library domain.
/// `LogicError` and `ParameterError` indicate programming mistakes, while `RuntimeError`, `OverflowError`, and
/// `ParseError` describe failures that can arise from data or the execution environment.
void builtInExceptions() {
    const auto parameterError = el::ParameterError{"The volume must be positive."_el, "volume"_el};
    const auto parseError = el::ParseError{"The musical note could not be parsed."_el, el::CpIndex{4}};
    const auto overflowError = el::OverflowError{"The time-signature counter exceeded its limit."_el};

    el::io::printLine("Parameter: "_el, parameterError.toString());
    el::io::printLine("Parse: "_el, parseError.toString());
    el::io::printLine("Overflow: "_el, overflowError.toString());
}

}
