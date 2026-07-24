// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unit/CodeLocation.hpp>
#include <erbsland/unit/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(CodeLocation)
class CodeLocationTest final : public el::UnitTest {
public:
    void testUndefinedLocation() {
        const auto location = el::unit::CodeLocation{};
        REQUIRE(location.isUndefined());
        REQUIRE(location.line().isNoIndex());
        REQUIRE(location.column().isNoIndex());
        REQUIRE(location.position().isNoIndex());
        REQUIRE_EQUAL(location.toString(), "undefined"_el);
    }

    void testConstructionAccessorsAndSetters() {
        const auto location = el::unit::CodeLocation{el::unit::LineIndex{2U}, el::unit::ColumnIndex{6U}};
        REQUIRE_EQUAL(location.line(), el::unit::LineIndex{2U});
        REQUIRE_EQUAL(location.column(), el::unit::ColumnIndex{6U});
        REQUIRE(location.position().isNoIndex());
        REQUIRE_EQUAL(location.toString(), "3:7"_el);

        auto changed = el::unit::CodeLocation{};
        changed.setLine(el::unit::LineIndex{4U})
            .setColumn(el::unit::ColumnIndex{8U})
            .setPosition(el::unit::CpIndex{12U});
        REQUIRE_EQUAL(changed.line(), el::unit::LineIndex{4U});
        REQUIRE_EQUAL(changed.column(), el::unit::ColumnIndex{8U});
        REQUIRE_EQUAL(changed.position(), el::unit::CpIndex{12U});
    }

    void testEqualityAndAdvancing() {
        auto location = el::unit::CodeLocation{};
        location.nextLine();
        REQUIRE_EQUAL(location.line(), el::unit::LineIndex::zero());
        REQUIRE_EQUAL(location.column(), el::unit::ColumnIndex::zero());
        REQUIRE_EQUAL(location.position(), el::unit::CpIndex::zero());
        REQUIRE_EQUAL(location.toString(), "1:1"_el);

        location.nextColumn();
        REQUIRE_EQUAL(location.column(), el::unit::ColumnIndex{1U});
        REQUIRE_EQUAL(location.position(), el::unit::CpIndex{1U});
        REQUIRE_EQUAL(location.toString(), "1:2"_el);

        location.nextLine();
        REQUIRE_EQUAL(location.line(), el::unit::LineIndex{1U});
        REQUIRE_EQUAL(location.column(), el::unit::ColumnIndex::zero());
        REQUIRE_EQUAL(location.position(), el::unit::CpIndex{2U});
        REQUIRE_EQUAL(location.toString(), "2:1"_el);
    }
};
