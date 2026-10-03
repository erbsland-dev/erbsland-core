// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Application.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Resources.hpp>

#ifdef ERBSLAND_CORE_BUILDING_LIBRARY
#error "Installed consumers must not inherit the library build marker."
#endif
#ifdef ERBSLAND_CORE_DO_NOT_FLATTEN_NS
#error "Installed consumers must receive the flattened namespace by default."
#endif

using namespace el::text::literals;

auto main() -> int {
    auto application = el::Application{};
    const auto &resources = application.resources();
    if (!resources.contains("installed"_el, "message.txt"_el)) {
        return 1;
    }
    if (resources.getTextOrThrow("installed"_el, "message.txt"_el) != "Installed package resource.\n"_el) {
        return 2;
    }
    return 0;
}
