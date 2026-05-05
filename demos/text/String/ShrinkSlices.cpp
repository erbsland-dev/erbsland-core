// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// `shrinkToFit()` is a deliberate compaction step, not routine cleanup.
///
/// A sliced string can keep the original backing store alive. Calling
/// `shrinkToFit()` materializes the visible range into exact-sized storage.
/// This is useful before keeping a small slice for a long time, but it should
/// not be used after every edit.
void shrinkSlices() {
    auto archiveLine =
        el::String{"Observatory log | Luzula sylvatica | vallée alpine | cielo sereno | 2026-06-07"_el};
    archiveLine.reserve(el::ByteLength{180U});

    const auto marker = "Luzula sylvatica"_el;
    auto specimen = archiveLine.slice(el::ByteRange{archiveLine.find(marker), marker.length()});

    el::io::printLine("Original line:"_el);
    el::io::printLine(archiveLine);
    el::io::printLine();

    el::io::printLine("Slice before compaction:"_el);
    el::io::printLine("  text ........: "_el, specimen);
    el::io::printLine("  length ......: "_el, specimen.length());
    el::io::printLine("  capacity ....: "_el, specimen.capacity());
    el::io::printLine("  memory usage : "_el, specimen.memoryUsage());

    specimen.shrinkToFit();

    el::io::printLine();
    el::io::printLine("Slice after shrinkToFit:"_el);
    el::io::printLine("  text ........: "_el, specimen);
    el::io::printLine("  length ......: "_el, specimen.length());
    el::io::printLine("  capacity ....: "_el, specimen.capacity());
    el::io::printLine("  memory usage : "_el, specimen.memoryUsage());
}
