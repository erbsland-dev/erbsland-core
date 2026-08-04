// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/Location.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <stdexcept>
#include <utility>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(Location)
class LocationTest final : public el::UnitTest {
public:
    void testDefaultConstructor() {
        Location loc;
        REQUIRE(loc.isUndefined());
        REQUIRE_EQUAL(loc.sourceIdentifier(), nullptr);
        REQUIRE(loc.codeLocation().isUndefined());
    }

    void testParameterizedConstructor() {
        const auto sourceIdentifier = SourceIdentifier::createForFile("file.elcl"_el);
        Location loc(sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{41U}, el::unit::ColumnIndex{9U}});
        REQUIRE_FALSE(loc.isUndefined());
        REQUIRE(SourceIdentifier::areEqual(loc.sourceIdentifier(), sourceIdentifier));
        REQUIRE_EQUAL(loc.codeLocation().line(), el::unit::LineIndex{41U});
        REQUIRE_EQUAL(loc.codeLocation().column(), el::unit::ColumnIndex{9U});
    }

    void testEqualityOperators() {
        Location loc1; // Undefined
        Location loc2; // Undefined

        // Two undefined locations should be equal
        REQUIRE_EQUAL(loc1, loc2);

        // Two undefined locations should not be unequal
        REQUIRE_EQUAL(loc1, loc2);

        const auto sourceIdentifier = SourceIdentifier::createForFile("file.elcl"_el);
        Location loc3(sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{9U}, el::unit::ColumnIndex{19U}});
        Location loc4(sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{9U}, el::unit::ColumnIndex{19U}});

        // Locations with the same data should be equal
        REQUIRE_EQUAL(loc3, loc4);

        // Locations with the same data should not be unequal
        REQUIRE_EQUAL(loc3, loc4);

        Location loc5(sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{9U}, el::unit::ColumnIndex{20U}});

        // Locations with different columns should be unequal
        REQUIRE_NOT_EQUAL(loc3, loc5);

        Location loc6(sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{10U}, el::unit::ColumnIndex{19U}});

        // Locations with different lines should be unequal
        REQUIRE_NOT_EQUAL(loc3, loc6);

        const auto sourceIdentifier2 = SourceIdentifier::createForFile("another_file.elcl"_el);
        Location loc7(sourceIdentifier2, el::unit::CodeLocation{el::unit::LineIndex{9U}, el::unit::ColumnIndex{19U}});

        // Locations with different source identifiers should be unequal
        REQUIRE_NOT_EQUAL(loc3, loc7);
    }

    void testCopyConstructor() {
        const auto sourceIdentifier = SourceIdentifier::createForFile("file.elcl"_el);
        Location original(
            sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{29U}, el::unit::ColumnIndex{39U}});
        Location copy = original;

        // The copied location should be equal to the original
        REQUIRE_EQUAL(copy, original);
    }

    void testMoveConstructor() {
        const auto sourceIdentifier = SourceIdentifier::createForFile("file.elcl"_el);
        Location original(
            sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{49U}, el::unit::ColumnIndex{59U}});
        Location moved = std::move(original);

        // Moved location should retain the source identifier
        REQUIRE_EQUAL(moved.sourceIdentifier()->name(), "file"_el);
        REQUIRE_EQUAL(moved.sourceIdentifier()->path(), "file.elcl"_el);

        // Moved location should retain the line number
        REQUIRE_EQUAL(moved.codeLocation().line(), el::unit::LineIndex{49U});

        // Moved location should retain the column number
        REQUIRE_EQUAL(moved.codeLocation().column(), el::unit::ColumnIndex{59U});

        // Note: The state of 'original' after move is unspecified
    }

    void testCopyAssignment() {
        const auto sourceIdentifier = SourceIdentifier::createForFile("file.elcl"_el);
        Location loc1(sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{69U}, el::unit::ColumnIndex{79U}});
        Location loc2;
        loc2 = loc1;

        // After copy assignment, loc2 should be equal to loc1
        REQUIRE_EQUAL(loc2, loc1);
    }

    void testMoveAssignment() {
        const auto sourceIdentifier = SourceIdentifier::createForFile("file2.elcl"_el);
        Location loc1(sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{89U}, el::unit::ColumnIndex{99U}});
        Location loc2;
        loc2 = std::move(loc1);

        // Move-assigned location should have the correct source identifier
        REQUIRE_EQUAL(loc2.sourceIdentifier()->path(), "file2.elcl"_el);

        // Move-assigned location should have the correct line number
        REQUIRE_EQUAL(loc2.codeLocation().line(), el::unit::LineIndex{89U});

        // Move-assigned location should have the correct column number
        REQUIRE_EQUAL(loc2.codeLocation().column(), el::unit::ColumnIndex{99U});
    }

    void testAccessors() {
        const auto sourceIdentifier = SourceIdentifier::createForFile("source.elcl"_el);
        Location loc(sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{14U}, el::unit::ColumnIndex{24U}});

        // Check sourceIdentifier accessor
        REQUIRE_EQUAL(loc.sourceIdentifier()->path(), "source.elcl"_el);

        // Check line accessor
        REQUIRE_EQUAL(loc.codeLocation().line(), el::unit::LineIndex{14U});

        // Check column accessor
        REQUIRE_EQUAL(loc.codeLocation().column(), el::unit::ColumnIndex{24U});
    }

    void testToText() {
        const auto sourceIdentifier = SourceIdentifier::createForFile("config.elcl"_el);
        Location loc(sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{4U}, el::unit::ColumnIndex{9U}});
        el::text::String expected = "file:config.elcl:5:10"_el;

        // Check if toText returns the correct formatted string
        REQUIRE_EQUAL(loc.toText(), expected);

        Location undefinedLoc;

        // Check if toText correctly represents an undefined location
        REQUIRE_EQUAL(undefinedLoc.toText(), "<unknown>"_el);
    }
};
