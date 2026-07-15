// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ErrorDocumentBuilder.hpp>
#include <erbsland/i18n/DisplayTextMap.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/TextNode.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>

using namespace el::text::literals;

TESTED_TARGETS(ErrorDocumentBuilder)
class ErrorDocumentBuilderTest final : public el::UnitTest {
public:
    void testInitialDocumentAndRootAccess() {
        auto builder = el::err::ErrorDocumentBuilder{"Failure"_el, "Detailed description."_el};
        builder.root()->addParagraph()->addText("Custom details."_el);
        auto document = builder.takeDocument();

        REQUIRE_EQUAL(document.root()->style(), "error"_el);
        const auto text = document.toString();
        REQUIRE(text.contains("Failure"_el));
        REQUIRE(text.contains("Detailed description."_el));
        REQUIRE(text.contains("Custom details."_el));
    }

    void testUnknownTitleAndSourceFields() {
        auto map = el::i18n::DisplayTextMap::defaultMap()->clone();
        map->set("UnknownError"_el, "Unspecified Failure"_el);
        auto builder = el::err::ErrorDocumentBuilder{{}, {}, map};
        builder.addSource(
            "configuration"_el,
            "/tmp/app.conf"_el,
            {.line = el::unit::LineIndex{1U}, .column = el::unit::ColumnIndex{2U}, .position = el::unit::CpIndex{3U}});
        const auto text = builder.takeDocument().toString();

        REQUIRE(text.contains("Unspecified Failure"_el));
        REQUIRE(text.contains("Error Source"_el));
        REQUIRE(text.contains("Source:"_el));
        REQUIRE(text.contains("configuration"_el));
        REQUIRE(text.contains("Path:"_el));
        REQUIRE(text.contains("/tmp/app.conf"_el));
        REQUIRE(text.contains("Line:"_el));
        REQUIRE(text.contains("Column:"_el));
        REQUIRE(text.contains("Position:"_el));
    }

    void testEmptySourceAddsNothing() {
        auto builder = el::err::ErrorDocumentBuilder{"Failure"_el};
        builder.addSource({}, {}, {});
        REQUIRE_FALSE(builder.takeDocument().toString().contains("Error Source"_el));
    }
};
