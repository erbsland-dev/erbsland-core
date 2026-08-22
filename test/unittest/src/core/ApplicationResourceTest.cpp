// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/resource/Resources.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Application ApplicationData ApplicationDataImpl ResourceManager)
class ApplicationResourceTest final : public el::UnitTest {
public:
    void testApplicationResourceAccess() {
        auto scope = ApplicationTestScope<>{};
        const auto &first = scope.app().resources();
        const auto &second = scope.app().resources();
        REQUIRE_EQUAL(&first, &second);
        REQUIRE(first.contains("test"_el, "plain.txt"_el));
    }
};
