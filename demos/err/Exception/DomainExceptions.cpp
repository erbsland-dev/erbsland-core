// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Domain-specific exceptions add meaning and diagnostic data while preserving the common `RuntimeError` contract.
/// Catch the most specific type when you can recover from that domain failure, or `RuntimeError` at a wider boundary.
void domainExceptions() {
    // An integer field rejects a text argument and raises the text-domain exception.
    try {
        const auto description = el::StringFormat{"Tempo: {:d}"_el}.build("速い"_el);
        el::io::printLine(description);
    } catch (const el::FormatError &error) {
        el::io::printLine("Text-domain error: "_el, error.reason());
    }

    // A child that was never created raises the path-domain exception when read.
    try {
        const auto missingScore = el::Path::currentDirectory().joined("__erbsland_missing_存在しない楽譜__.music"_el);
        const auto score = missingScore.content().readTextOrThrow();
        el::io::printLine(score);
    } catch (const el::PathError &error) {
        el::io::printLine("Path-domain error: "_el, error.reason());
    }
}

}
