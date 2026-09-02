// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/impl/CodeSnippetLayout.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unit/ColumnCount.hpp>
#include <erbsland/unit/ColumnIndex.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string_view>
#include <vector>

using namespace el::text;
using namespace el::text::literals;
using namespace el::unit;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(CodeSnippetLayout CodeSnippetLayoutCell CodeSnippetLayoutMarker CodeSnippetLayoutRow)
class CodeSnippetLayoutTest final : public el::UnitTest {
    using CodeSnippetLayout = el::text::impl::CodeSnippetLayout;
    using CodeSnippetLayoutCell = el::text::impl::CodeSnippetLayoutCell;
    using CodeSnippetLayoutMarker = el::text::impl::CodeSnippetLayoutMarker;
    using CodeSnippetLayoutRow = el::text::impl::CodeSnippetLayoutRow;

private:
    [[nodiscard]] static auto rowText(const CodeSnippetLayoutRow &row) -> String {
        auto result = StringEditor{};
        for (const auto &cell : row.cells()) {
            result.append(cell.text);
        }
        return result;
    }

public:
    void testCropsUnmarkedSource() {
        const auto layout = CodeSnippetLayout{"abcdef"_el, {}, 4};

        REQUIRE_EQUAL(layout.rows().size(), std::size_t{1U});
        REQUIRE_EQUAL(layout.rows().front().displayWidth(), 4);
        REQUIRE_EQUAL(rowText(layout.rows().front()), "abc…"_el);
        REQUIRE(layout.rows().front().cells().back().isEllipsis);
    }

    void testWrapsWideCombiningAndControlCharacters() {
        const auto sourceBytes = th::stdStringFromHex("41 E7 95 8C CC 81 09 42");
        const auto source = StringEditor{std::string_view{sourceBytes}};
        const auto markers = std::vector<CodeSnippetLayoutMarker>{
            {{ColumnIndex{1U}, ColumnCount{2U}}, "wide"_el},
            {{ColumnIndex{3U}, ColumnCount{}}, "point"_el},
        };
        const auto layout = CodeSnippetLayout{source, markers, 3};

        REQUIRE_EQUAL(layout.rows().size(), std::size_t{2U});
        REQUIRE_EQUAL(layout.rows()[0].displayWidth(), 3);
        REQUIRE_EQUAL(layout.rows()[1].displayWidth(), 2);
        REQUIRE_EQUAL(rowText(layout.rows()[1]), "?B"_el);

        const auto wide = layout.rows()[0].markerPlacement(markers[0].range);
        REQUIRE(wide.has_value());
        REQUIRE_EQUAL(wide->start, 1);
        REQUIRE_EQUAL(wide->length, 2);
        const auto point = layout.rows()[1].markerPlacement(markers[1].range);
        REQUIRE(point.has_value());
        REQUIRE_EQUAL(point->start, 0);
        REQUIRE_EQUAL(point->length, 1);
    }

    void testRetainsFiveRowsAroundMarkers() {
        const auto markerRange = ColumnRange{ColumnIndex{10U}, ColumnCount{4U}};
        const auto markers = std::vector<CodeSnippetLayoutMarker>{{markerRange, "range"_el}};
        const auto layout = CodeSnippetLayout{"abcdefghijklmnopqrst"_el, markers, 2};

        REQUIRE_EQUAL(layout.rows().size(), std::size_t{5U});
        REQUIRE_EQUAL(rowText(layout.rows().front()), "…j"_el);
        REQUIRE_EQUAL(rowText(layout.rows().back()), "q…"_el);
        REQUIRE(layout.rows().front().cells().front().isEllipsis);
        REQUIRE(layout.rows().back().cells().back().isEllipsis);
        REQUIRE(layout.rows()[1].markerPlacement(markerRange).has_value());
        REQUIRE(layout.rows()[2].markerPlacement(markerRange).has_value());
        REQUIRE_FALSE(layout.isLastMarkerRow(1U, markerRange));
        REQUIRE(layout.isLastMarkerRow(2U, markerRange));
    }

    void testPlacesPointInEmptySource() {
        const auto point = ColumnRange{ColumnIndex::zero(), ColumnCount{}};
        const auto markers = std::vector<CodeSnippetLayoutMarker>{{point, "empty"_el}};
        const auto layout = CodeSnippetLayout{String{}, markers, 8};

        REQUIRE_EQUAL(layout.rows().size(), std::size_t{1U});
        const auto placement = layout.rows().front().markerPlacement(point);
        REQUIRE(placement.has_value());
        REQUIRE_EQUAL(placement->start, 0);
        REQUIRE_EQUAL(placement->length, 1);
    }
};
