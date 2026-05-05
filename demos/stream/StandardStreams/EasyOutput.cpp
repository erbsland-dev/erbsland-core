// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardStreamsDemos.hpp"


void easyOutput() {
    el::io::printLine("Forest survey started."_el);
    el::io::printLine("Plot "_el, 17, ": "_el, "north ridge"_el);
    el::io::printLine("Healthy trees: "_el, 42, ", damaged trees: "_el, 3);
}
