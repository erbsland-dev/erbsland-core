// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>

using el::unit::CodeLocation;
using namespace el::text::literals;

TESTED_TARGETS(Document Value Location el::unit::CodeLocation)
class ValueLocationTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
public:
    void requireLocation(const ConstValuePtr &testedValue, const std::size_t line, const std::size_t column) {
        const auto location = testedValue->location().codeLocation();
        REQUIRE_EQUAL(location.line(), el::unit::LineIndex::fromSizeT(line));
        REQUIRE_EQUAL(location.column(), el::unit::ColumnIndex::fromSizeT(column));
        REQUIRE_FALSE(location.position().isNoIndex());
    }

    TESTED_TARGETS(hasLocation location setLocation)
    void testLocation() {
        setupTemplate1("1");
        for (const auto &[namePath, value] : doc->toFlatValueMap()) {
            REQUIRE(value->hasLocation());
            REQUIRE(!value->location().codeLocation().isUndefined());
            const auto location = value->location();
            const auto sourceName = location.sourceIdentifier()->name();
            REQUIRE_EQUAL(sourceName, el::text::String{"text"_el});
            REQUIRE(value->location().sourceIdentifier()->path().isEmpty());
        }
        value = doc->valueOrThrow(el::text::String{"main"});
        WITH_CONTEXT(requireLocation(value, 0U, 0U));
        value = doc->valueOrThrow(el::text::String{"main.value1"});
        WITH_CONTEXT(requireLocation(value, 1U, 0U));
        value = doc->valueOrThrow(el::text::String{"main.value_list"});
        WITH_CONTEXT(requireLocation(value, 5U, 0U));
        value = doc->valueOrThrow(el::text::String{"main.sub.sub.a"});
        WITH_CONTEXT(requireLocation(value, 15U, 0U));
        value = doc->valueOrThrow(el::text::String{"list"});
        WITH_CONTEXT(requireLocation(value, 21U, 0U));
        value = doc->valueOrThrow(el::text::String{"list[0]"});
        WITH_CONTEXT(requireLocation(value, 21U, 0U));
        value = doc->valueOrThrow(el::text::String{"list[1]"});
        WITH_CONTEXT(requireLocation(value, 23U, 0U));
        value = doc->valueOrThrow(el::text::String{"list[2]"});
        WITH_CONTEXT(requireLocation(value, 25U, 0U));
        value = doc->valueOrThrow(el::text::String{"list[2].value"});
        WITH_CONTEXT(requireLocation(value, 26U, 0U));
        value = doc->valueOrThrow(el::text::String{"main.text"});
        WITH_CONTEXT(requireLocation(value, 27U, 0U));
        value = doc->valueOrThrow(el::text::String{"main.text.\"first\""});
        WITH_CONTEXT(requireLocation(value, 28U, 0U));
        value = doc->valueOrThrow(el::text::String{"main.text.\"second\""});
        WITH_CONTEXT(requireLocation(value, 29U, 0U));
        value = doc->valueOrThrow(el::text::String{"main.sub_text"});
        WITH_CONTEXT(requireLocation(value, 31U, 0U));
        value = doc->valueOrThrow(el::text::String{"main.sub_text.\"first\""});
        WITH_CONTEXT(requireLocation(value, 31U, 0U));

        value = doc->valueOrThrow(el::text::String{"main.value_list[0]"});
        WITH_CONTEXT(requireLocation(value, 5U, 13U));
        value = doc->valueOrThrow(el::text::String{"main.value_list[1]"});
        WITH_CONTEXT(requireLocation(value, 5U, 16U));
        value = doc->valueOrThrow(el::text::String{"main.value_list[2]"});
        WITH_CONTEXT(requireLocation(value, 5U, 19U));

        value = doc->valueOrThrow(el::text::String{"main.value_matrix"});
        WITH_CONTEXT(requireLocation(value, 7U, 0U));
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[0]"});
        WITH_CONTEXT(requireLocation(value, 8U, 4U));
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[0][0]"});
        WITH_CONTEXT(requireLocation(value, 8U, 6U));
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[0][1]"});
        WITH_CONTEXT(requireLocation(value, 8U, 9U));
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[0][2]"});
        WITH_CONTEXT(requireLocation(value, 8U, 12U));
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[1]"});
        WITH_CONTEXT(requireLocation(value, 9U, 4U));
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[1][0]"});
        WITH_CONTEXT(requireLocation(value, 9U, 6U));
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[1][1]"});
        WITH_CONTEXT(requireLocation(value, 9U, 9U));
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[1][2]"});
        WITH_CONTEXT(requireLocation(value, 9U, 12U));
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[2]"});
        WITH_CONTEXT(requireLocation(value, 10U, 4U));
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[2][0]"});
        WITH_CONTEXT(requireLocation(value, 10U, 6U));
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[2][1]"});
        WITH_CONTEXT(requireLocation(value, 10U, 9U));
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[2][2]"});
        WITH_CONTEXT(requireLocation(value, 10U, 12U));
    }

    void testValueListKeepsLocation() {
        el::text::String documentText = "[main]\nvalue = \n    1, 2, 3\n"_el;
        Parser parser;
        REQUIRE_NOTHROW(doc = parser.parseOrThrow(Source::fromString(documentText)));
        WITH_CONTEXT(requireLocation(doc->valueOrThrow(el::text::String{"main.value"}), 1U, 0U));
        WITH_CONTEXT(requireLocation(doc->valueOrThrow(el::text::String{"main.value[0]"}), 2U, 4U));
    }
};
