// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/Application.hpp>
#include <erbsland/resource/Resources.hpp>
#include <erbsland/text/Literals.hpp>

using namespace el::text::literals;

auto main() -> int {
    auto application = el::core::Application{};
    const auto &resources = application.resources();
    if (!resources.contains("installed"_el, "message.txt"_el)) {
        return 1;
    }
    if (resources.getTextOrThrow("installed"_el, "message.txt"_el) != "Installed package resource.\n"_el) {
        return 2;
    }
    return 0;
}
