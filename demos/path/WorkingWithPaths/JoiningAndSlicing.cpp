// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "WorkingWithPathsDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

/// Join paths and extract element ranges.
///
/// Joining appends the non-root elements of the right-hand path, even when that path is absolute. Slicing works on the
/// public element sequence. If the slice starts with the root element, the result remains absolute; otherwise it
/// becomes a relative path.
void joiningAndSlicing() {
    const auto archive = el::Path{"/arquivo/expedicoes"_el};
    const auto morningLog = archive / "ceu-leste"_el / "manha.txt"_el;
    const auto borrowedAbsolute = archive / el::Path{"/rotas/noite.txt"_el};

    // Build paths from reusable fragments.
    el::io::printLine("joined ............: "_el, morningLog.toString());
    el::io::printLine("absolute rhs ......: "_el, borrowedAbsolute.toString());

    // Slice with and without the root element.
    const auto absoluteSlice = morningLog.slice(el::ElementRange{el::ElementIndex{0}, el::ElementCount{3}});
    const auto relativeSlice = morningLog.slice(el::ElementRange{el::ElementIndex{1}, el::ElementCount{2}});

    el::io::printLine("absolute slice ....: "_el, absoluteSlice.toString());
    el::io::printLine("relative slice ....: "_el, relativeSlice.toString());
}

}
