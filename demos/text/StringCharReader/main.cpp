// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringCharReaderDemos.hpp"

#include <DemoCommon.hpp>

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("StudyPlanParser"_el, studyPlanParser);
    return app.run();
}
