// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// An exception exposes text for different consumers.
/// `reason()` is the stable reason supplied by the thrower, `toString()` may add exception-specific details, and
/// `what()` provides the null-terminated compatibility string expected by standard C++ interfaces.
void exceptionText() {
    try {
        const auto tempo = el::String{"速い"_el}.toIntegerOrThrow<int>();
        el::io::printLine("Tempo: "_el, tempo);
    } catch (const el::ParseError &error) {
        el::io::printLine("reason(): "_el, error.reason());
        el::io::printLine("toString(): "_el, error.toString());
        el::io::printLine("what(): "_el, error.what());
    }
}

}
