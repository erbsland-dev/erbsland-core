// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardStreamsDemos.hpp"


void standardOutputAndError() {
    const auto out = el::stdOut();
    const auto err = el::stdErr();

    out->printLine("Writing regular progress to standard output."_el);
    err->printLine("This warning goes to standard error: missing optional humidity sensor."_el);
}
