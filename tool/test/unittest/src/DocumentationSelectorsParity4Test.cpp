// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <tool/doc/impl/SourceSelectors.hpp>

using namespace el::text::literals;
TESTED_TARGETS(SourceSelectors)
class DocumentationSelectorsParity4Test final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCase19() {
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
            selector.select(R"FIXTURE(c:"Intro, then start"-a:value)FIXTURE"_el, R"FIXTURE()FIXTURE"_el).join("\n"_el),
            R"FIXTURE(// Intro, then start
auto value =
    42;)FIXTURE"_el);
    }
    void testCase20() {
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
            selector.select(R"FIXTURE(f:second, f:first, f:second)FIXTURE"_el, R"FIXTURE()FIXTURE"_el).join("\n"_el),
            R"FIXTURE(void second() {
    finish();
}

void first() {
    // detail
    work();
})FIXTURE"_el);
    }
    void testCase21() {
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
            selector.select(R"FIXTURE(f:first, c:detail)FIXTURE"_el, R"FIXTURE()FIXTURE"_el).join("\n"_el),
            R"FIXTURE(void first() {
    // detail
    work();
})FIXTURE"_el);
    }
};
