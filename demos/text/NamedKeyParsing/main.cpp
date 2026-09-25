// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "NamedKeyParsingDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ChooseEntryPolicies"_el, chooseEntryPolicies);
    app.registerDemo("ChooseSeparators"_el, chooseSeparators);
    app.registerDemo("LimitAnswerValues"_el, limitAnswerValues);
    app.registerDemo("ParseFactRequest"_el, parseFactRequest);
    app.registerDemo("PrefixAndCompactValues"_el, prefixAndCompactValues);
    app.registerDemo("RegisterQuizKeys"_el, registerQuizKeys);
    app.registerDemo("RestrictValueCharacters"_el, restrictValueCharacters);
    return app.run();
}
