// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "WorkingWithPathsDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

/// Inspect roots, elements, names, suffixes, and parent paths.
///
/// A path can be queried without touching the filesystem. Roots are part of the public element sequence, while names,
/// stems, and suffixes operate on the final non-root element. This makes metadata-style path work deterministic and
/// independent from the current operating system.
void inspectingPaths() {
    const auto report = el::Path{"/arquivo/rotas/relatorio.final.txt"_el};

    // Basic structure and naming information.
    el::io::printLine("path ..............: "_el, report.toString());
    el::io::printLine("root ..............: "_el, report.root());
    el::io::printLine("name ..............: "_el, report.name());
    el::io::printLine("stem ..............: "_el, report.stem());
    el::io::printLine("suffixes ..........: "_el, report.suffixes());

    // Public elements include the root for absolute paths.
    for (auto index = el::ItemIndex{}; index.isWithin(report.elementCount()); ++index) {
        el::io::printLine("element "_el, index.toSizeT(), " .........: "_el, report.element(index));
    }

    // Parent paths are ordered from nearest to furthest.
    for (const auto &parent : report.parents()) {
        el::io::printLine("parent ............: "_el, parent.toString());
    }
}

}
