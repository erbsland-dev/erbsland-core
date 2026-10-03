// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <tool/doc/impl/SourceSelectors.hpp>

using namespace el::text::literals;
TESTED_TARGETS(SourceSelectors)
class DocumentationSelectorsParity1Test final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCase1() {
        const auto selector = tool::doc::impl::SourceSelectors{R"FIXTURE(print("value=wrong;");
// Say "hello"
auto value = "a;b";)FIXTURE"_el};
        REQUIRE_EQUAL(
            selector.select(R"FIXTURE(c:hello)FIXTURE"_el, R"FIXTURE()FIXTURE"_el).join("\n"_el),
            R"FIXTURE(// Say "hello")FIXTURE"_el);
    }
    void testCase2() {
        const auto selector = tool::doc::impl::SourceSelectors{R"FIXTURE(print("value=wrong;");
// Say "hello"
auto value = "a;b";)FIXTURE"_el};
        REQUIRE_EQUAL(
            selector.select(R"FIXTURE(a:value)FIXTURE"_el, R"FIXTURE()FIXTURE"_el).join("\n"_el),
            R"FIXTURE(auto value = "a;b";)FIXTURE"_el);
    }
    void testCase3() {
        const auto selector = tool::doc::impl::SourceSelectors{R"FIXTURE(auto url = "https://detail";
// detail)FIXTURE"_el};
        REQUIRE_EQUAL(
            selector.select(R"FIXTURE(c:detail)FIXTURE"_el, R"FIXTURE()FIXTURE"_el).join("\n"_el),
            R"FIXTURE(// detail)FIXTURE"_el);
    }
    void testCase4() {
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
            selector.select(R"FIXTURE(f:first)FIXTURE"_el, R"FIXTURE(c:detail)FIXTURE"_el).join("\n"_el),
            R"FIXTURE(void first() {

    work();
})FIXTURE"_el);
    }
    void testCase5() {
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
            selector.select(R"FIXTURE(f:first)FIXTURE"_el, R"FIXTURE(f:first(-0,-3), f:first(-3,-0))FIXTURE"_el)
                .join("\n"_el),
            R"FIXTURE(    // detail
    work();)FIXTURE"_el);
    }
    void testCase6() {
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
            selector.select(R"FIXTURE(f:first)FIXTURE"_el, R"FIXTURE(f:first)FIXTURE"_el).join("\n"_el),
            R"FIXTURE()FIXTURE"_el);
    }
};
