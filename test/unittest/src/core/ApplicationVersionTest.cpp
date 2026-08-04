// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/Application.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Application)
class ApplicationVersionTest final : public el::UnitTest {
public:
    void testLibraryVersion() {
        const auto version = el::core::Application::libraryVersion();

        REQUIRE_GREATER_EQUAL(version, el::unit::Version{});
        const auto currentVersion = el::core::Application::libraryVersion();
        REQUIRE_EQUAL(currentVersion, version);
    }

    void testLibraryVersionText() {
        const auto versionText = el::core::Application::libraryVersionText();

        REQUIRE_FALSE(versionText.isEmpty());
        REQUIRE_FALSE(versionText.contains("@ERBSLAND_CORE_VERSION_TEXT@"_el));
        REQUIRE_FALSE(versionText.contains("@ERBSLAND_CORE_VERSION"_el));
    }
};
