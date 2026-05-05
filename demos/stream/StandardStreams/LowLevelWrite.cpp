// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardStreamsDemos.hpp"


void lowLevelWrite() {
    const auto out = el::stdOut();

    out->write("Low-level write keeps already-built text on the direct path."_el);
    out->write(el::Char{U'\n'});
    out->write("Use it when producing larger text in chunks."_el);
    out->writeLine();
}
