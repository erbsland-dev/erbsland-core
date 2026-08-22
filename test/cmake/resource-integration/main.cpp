// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/Application.hpp>
#include <erbsland/resource/Resources.hpp>
#include <erbsland/text/Literals.hpp>

using namespace el::text::literals;

auto main() -> int {
    auto application = el::core::Application{};
    const auto &resources = application.resources();
    if (!resources.contains("fixture"_el, "alpha.txt"_el) ||
        !resources.contains("fixture"_el, "nested folder/café.json"_el) ||
        resources.contains("fixture"_el, "ignored.bin"_el)) {
        return 1;
    }
    if (resources.getTextOrThrow("fixture"_el, "nested folder/café.json"_el) != "{\"value\": 42}\n"_el) {
        return 2;
    }
    const auto compressedInfo = resources.getInfoOrThrow("fixture"_el, "alpha.txt"_el);
    if (!compressedInfo.isCompressed() || !compressedInfo.hasHash()) {
        return 3;
    }
    const auto rawInfo = resources.getInfoOrThrow("fixture"_el, "raw.txt"_el);
    if (rawInfo.isCompressed() || rawInfo.hasHash()) {
        return 4;
    }
    if (resources.getTextOrThrow("fixture"_el, "raw.txt"_el) != "Stored without transformations.\n"_el) {
        return 5;
    }
    return 0;
}
