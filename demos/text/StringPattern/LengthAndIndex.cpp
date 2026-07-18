// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `length()` and `index()` return native string positions for the matching part.
///
/// For UTF-8 text this means byte positions, for UTF-16 it means data-unit positions, and for UTF-32 it means
/// code-point positions. The values are meant for slicing and diagnostics near string boundaries, not for scanning an
/// entire grammar.
void lengthAndIndex() {
    const auto microSensor = el::StringPattern{u8"µ?*"_el};
    const auto u8Measurement = el::String{u8"µA=0.42"_el};

    el::io::printLine("UTF-8 split index .......: "_el, microSensor.index(u8Measurement));
    el::io::printLine("UTF-8 match bytes .......: "_el, microSensor.length(u8Measurement));

    const auto suffix = el::StringPattern{"*;ok"_el};
    const auto accepted = el::String{"temperatur=21.4;ok"_el};

    el::io::printLine("suffix index ............: "_el, suffix.index(accepted));
    el::io::printLine("suffix length ...........: "_el, suffix.length(accepted));
}

}
