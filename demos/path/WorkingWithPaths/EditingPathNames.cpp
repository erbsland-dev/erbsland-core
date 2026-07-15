// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "WorkingWithPathsDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

/// Edit the final path element without rebuilding the whole path string.
///
/// `withName()`, `withSuffix()`, and `withStem()` return modified paths and leave the original path unchanged. They
/// are useful when an application derives companion files, report formats, or archive names from a common base path.
void editingPathNames() {
    const auto baseReport = el::Path{"expedicao/diario/vento.final.txt"_el};

    // Create related names from one base report.
    const auto publicReport = baseReport.withName("relatorio.txt"_el);
    const auto markdownReport = baseReport.withSuffix("md"_el);
    const auto archiveReport = baseReport.withStem("arquivo-vento"_el);
    const auto plainName = baseReport.withSuffix({}); // remove all suffixes

    el::io::printLine("base ..............: "_el, baseReport.toString());
    el::io::printLine("name ..............: "_el, publicReport.toString());
    el::io::printLine("suffix ............: "_el, markdownReport.toString());
    el::io::printLine("stem ..............: "_el, archiveReport.toString());
    el::io::printLine("no suffix .........: "_el, plainName.toString());
}

}
