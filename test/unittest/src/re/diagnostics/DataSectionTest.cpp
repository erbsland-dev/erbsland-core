// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/re/impl/diagnostics/DataSection.hpp>
#include <erbsland/re/StdFormatForRegEx.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using namespace el::text::literals;
using impl::DataSection;

TESTED_TARGETS(DataSection)
TAGS(Diagnostics)
class DataSectionTest final : public el::UnitTest {
public:
    void testToString() {
        REQUIRE_EQUAL(impl::toString(DataSection::Program), "program"_el);
        REQUIRE_EQUAL(impl::toString(DataSection::Sequence), "sequence"_el);
        REQUIRE_EQUAL(impl::toString(DataSection::Class), "class"_el);
    }

    void testFormat() {
        REQUIRE_EQUAL(std::format("{}", DataSection::Program), "program");
        REQUIRE_EQUAL(std::format("{}", DataSection::Sequence), "sequence");
        REQUIRE_EQUAL(std::format("{}", DataSection::Class), "class");
    }

    void testToDataSection() {
        REQUIRE_EQUAL(impl::toDataSection("program"_el), DataSection::Program);
        REQUIRE_EQUAL(impl::toDataSection("sequence"_el), DataSection::Sequence);
        REQUIRE_EQUAL(impl::toDataSection("class"_el), DataSection::Class);
        REQUIRE_EQUAL(impl::toDataSection("unknown"_el), DataSection::Program); // Default
    }
};
