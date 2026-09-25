// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "FuzzyMatchingDemos.hpp"

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("SuggestSoundNames"_el, suggestSoundNames);
    app.registerDemo("TuneSuggestions"_el, tuneSuggestions);
    app.registerDemo("InspectMatches"_el, inspectMatches);
    return app.run();
}
