// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <tool/doc/impl/SourceSelectors.hpp>

using namespace el::text::literals;
TESTED_TARGETS(SourceSelectors)
class DocumentationSelectorsParity3Test final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCase13() {
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
        REQUIRE_THROWS(selector.select(R"FIXTURE(f:first,)FIXTURE"_el, R"FIXTURE()FIXTURE"_el));
    }
    void testCase14() {
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
        REQUIRE_THROWS(selector.select(R"FIXTURE(c:"unclosed)FIXTURE"_el, R"FIXTURE()FIXTURE"_el));
    }
    void testCase15() {
        const auto selector = tool::doc::impl::SourceSelectors{R"FIXTURE(// Intro, then start
auto value =
    42;

void first() {
    // detail
    work();
}

void second() {
    finish();
}
// detail again)FIXTURE"_el};
        REQUIRE_THROWS(selector.select(R"FIXTURE(c:detail)FIXTURE"_el, R"FIXTURE()FIXTURE"_el));
    }
    void testCase16() {
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
            selector.select(R"FIXTURE(c:"Intro, then start"(+0,+2))FIXTURE"_el, R"FIXTURE()FIXTURE"_el).join("\n"_el),
            R"FIXTURE(// Intro, then start
auto value =
    42;)FIXTURE"_el);
    }
    void testCase17() {
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
            selector.select(R"FIXTURE(a:value)FIXTURE"_el, R"FIXTURE()FIXTURE"_el).join("\n"_el), R"FIXTURE(auto value =
    42;)FIXTURE"_el);
    }
    void testCase18() {
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
            selector.select(R"FIXTURE(f:first(-1))FIXTURE"_el, R"FIXTURE()FIXTURE"_el).join("\n"_el),
            R"FIXTURE(// detail
work();)FIXTURE"_el);
    }
};
