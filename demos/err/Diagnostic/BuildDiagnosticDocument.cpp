// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <stdexcept>

namespace demo {

/// `DiagnosticHelper` converts an exception into one diagnostic or a complete document containing every cause.
/// Use `toDocument()` at a reporting boundary so valuable information from translated failures is not lost.
void buildDiagnosticDocument() {
    try {
        try {
            throw std::runtime_error{"MIDI input stopped responding"};
        } catch (...) {
            throw el::RuntimeError{"The digital-piano input could not be read."_el, std::current_exception()};
        }
    } catch (const el::Exception &error) {
        auto document = el::DiagnosticHelper{error}.toDocument();
        el::io::printLine(document.toString());
    }
}

}
