// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <tool/doc/impl/SourceSelectors.hpp>

using namespace el::text::literals;
TESTED_TARGETS(SourceSelectors)
class DocumentationSelectorsParity2Test final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCase7() {
        const auto selector = tool::doc::impl::SourceSelectors{R"FIXTURE(// Intro, then start
auto value =
    42;

void first() {
    // detail
    work();
}

void second() {
    finish();
})FIXTURE"_el};
        REQUIRE_EQUAL(
            selector.select(R"FIXTURE(c:detail(+1))FIXTURE"_el, R"FIXTURE()FIXTURE"_el).join("\n"_el),
            R"FIXTURE(void first() {
    // detail
    work();)FIXTURE"_el);
    }
    void testCase8() {
        const auto selector = tool::doc::impl::SourceSelectors{R"FIXTURE(// Intro, then start
auto value =
    42;

void first() {
    // detail
    work();
}

void second() {
    finish();
})FIXTURE"_el};
        REQUIRE_THROWS(selector.select(R"FIXTURE(f:first(-9))FIXTURE"_el, R"FIXTURE()FIXTURE"_el));
    }
    void testCase9() {
        const auto selector = tool::doc::impl::SourceSelectors{R"FIXTURE(// Intro, then start
auto value =
    42;

void first() {
    // detail
    work();
}

void second() {
    finish();
})FIXTURE"_el};
        REQUIRE_THROWS(selector.select(R"FIXTURE(f:first(+90))FIXTURE"_el, R"FIXTURE()FIXTURE"_el));
    }
    void testCase10() {
        const auto selector = tool::doc::impl::SourceSelectors{R"FIXTURE(// Intro, then start
auto value =
    42;

void first() {
    // detail
    work();
}

void second() {
    finish();
})FIXTURE"_el};
        REQUIRE_THROWS(selector.select(R"FIXTURE(f:second-f:first)FIXTURE"_el, R"FIXTURE()FIXTURE"_el));
    }
    void testCase11() {
        const auto selector = tool::doc::impl::SourceSelectors{R"FIXTURE(// Intro, then start
auto value =
    42;

void first() {
    // detail
    work();
}

void second() {
    finish();
})FIXTURE"_el};
        REQUIRE_THROWS(selector.select(R"FIXTURE(c:missing)FIXTURE"_el, R"FIXTURE()FIXTURE"_el));
    }
    void testCase12() {
        const auto selector = tool::doc::impl::SourceSelectors{R"FIXTURE(// Intro, then start
auto value =
    42;

void first() {
    // detail
    work();
}

void second() {
    finish();
})FIXTURE"_el};
        REQUIRE_THROWS(selector.select(R"FIXTURE(f:)FIXTURE"_el, R"FIXTURE()FIXTURE"_el));
    }
};
