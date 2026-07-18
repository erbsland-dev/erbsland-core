// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// A parser throws `ParseError` when its input violates the notation grammar.
auto parseDynamics(const el::String &text) -> el::String {
    if (text == "piano"_el || text == "forte"_el) {
        return text;
    }
    throw el::ParseError{"The dynamic marking must be 'piano' or 'forte'."_el, el::CpIndex{0}};
}

/// Catch the most specific recoverable exception close to the operation that understands it.
/// Broader runtime-error handlers belong at subsystem or application boundaries, where a generic fallback is useful.
void throwAndCatch() {
    try {
        el::io::printLine("Dynamics: "_el, parseDynamics("fortissimo"_el));
    } catch (const el::ParseError &error) {
        el::io::printLine("Please correct the notation: "_el, error.toString());
    } catch (const el::RuntimeError &error) {
        el::io::printLine("The score could not be loaded: "_el, error.reason());
    }
}

}
