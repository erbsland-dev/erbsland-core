// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "WorkingWithPathsDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

/// Convert paths back to display, POSIX, Windows, and standard-library forms.
///
/// `toString()` is the preferred platform-independent display form. `toPosixPath()` and `toWindowsPath()` are for
/// explicit external formats, while `toStdPath()` is reserved for interoperability with code that expects a standard
/// library path object.
void convertingPaths() {
    const auto relative = el::Path{"expedicao/rotas/manha.txt"_el};
    const auto windows = el::Path::fromWindows("C:\\Expedicao\\rotas\\manha.txt"_el);
    const auto posix = el::Path::fromPosix("/arquivo/rotas/manha.txt"_el);

    // Relative paths can be emitted in either text format.
    el::io::printLine("display ...........: "_el, relative.toString());
    el::io::printLine("posix   ...........: "_el, relative.toPosix());
    el::io::printLine("windows ...........: "_el, relative.toWindows(el::PathWindowsFormat::Native));

    // Absolute Windows and POSIX roots stay in their own external format.
    el::io::printLine("drive   ...........: "_el, windows.toWindows(el::PathWindowsFormat::Native));
    el::io::printLine("posix / ...........: "_el, posix.toPosix());

    // Use std::filesystem interop only when another API requires it.
    const auto stdPath = relative.toStdPath();
    el::io::printLine("std ...............: "_el, el::StringConverter{stdPath.generic_string()}.toString());
}

}
