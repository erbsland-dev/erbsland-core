// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "WorkingWithPathsDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

/// Construct paths from generic, POSIX, and Windows text.
///
/// `Path` stores platform-independent path data. Generic construction recognizes common root forms and normalizes
/// separators to slash characters. Use `fromPosix()` or `fromWindows()` when text comes from a known external format
/// and must not be interpreted through the generic auto-detection rules.
void constructionAndFormats() {
    // Generic construction normalizes repeated separators.
    const auto logBook = el::Path{"expedicao//ceu/diario.txt"_el};
    el::io::printLine("generic ...........: "_el, logBook.toString());

    // POSIX parsing treats a drive-looking prefix as ordinary relative text.
    const auto posixManifest = el::Path::fromPosix("c:/manifestos/vento.txt"_el);
    el::io::printLine("posix .............: "_el, posixManifest.toString());
    el::io::printLine("relative ..........: "_el, el::BooleanFormat::yesNo(), posixManifest.isRelative());

    // Windows parsing accepts backslashes and normalizes drive letters.
    const auto windowsChart = el::Path::fromWindows("C:\\Expedicao\\rotas\\manha.txt"_el);
    el::io::printLine("windows ...........: "_el, windowsChart.toString());

    // UNC roots keep the share name as written, while the server name is normalized.
    const auto sharedChart = el::Path::fromWindows("\\\\PORTO-CEU\\Mapas\\norte.txt"_el);
    el::io::printLine("unc root ..........: "_el, sharedChart.root());
}

}
