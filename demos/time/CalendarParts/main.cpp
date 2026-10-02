// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarPartsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Aggregates"_el, aggregates);
    app.registerDemo("Amounts"_el, amounts);
    app.registerDemo("Construct"_el, construct);
    app.registerDemo("Lengths"_el, lengths);
    app.registerDemo("Navigate"_el, navigate);
    app.registerDemo("Ordinal"_el, ordinal);
    app.registerDemo("Weekdays"_el, weekdays);
    return app.run();
}
