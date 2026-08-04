// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/re/impl/diagnostics/DataSection.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using namespace el::text::literals;
using impl::DataSection;

TESTED_TARGETS(DataSection)
TAGS(Diagnostics)
class DataSectionTest final : public el::UnitTest {
public:
    void testToString() {
        REQUIRE_EQUAL(el::re::impl::toString(DataSection::Program), "program"_el);
        REQUIRE_EQUAL(el::re::impl::toString(DataSection::Sequence), "sequence"_el);
        REQUIRE_EQUAL(el::re::impl::toString(DataSection::Class), "class"_el);
    }

    void testFormat() {
        const auto program = std::format("{}", DataSection::Program);
        const auto sequence = std::format("{}", DataSection::Sequence);
        const auto charClass = std::format("{}", DataSection::Class);
        REQUIRE_EQUAL(program, "program");
        REQUIRE_EQUAL(sequence, "sequence");
        REQUIRE_EQUAL(charClass, "class");
    }

    void testToDataSection() {
        REQUIRE_EQUAL(el::re::impl::toDataSection("program"_el), DataSection::Program);
        REQUIRE_EQUAL(el::re::impl::toDataSection("sequence"_el), DataSection::Sequence);
        REQUIRE_EQUAL(el::re::impl::toDataSection("class"_el), DataSection::Class);
        REQUIRE_EQUAL(el::re::impl::toDataSection("unknown"_el), DataSection::Program); // Default
    }
};
