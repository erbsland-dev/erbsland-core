// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/debug/impl/StringDebugBuilder.hpp>
#include <erbsland/text/impl/StringStorageKind.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>

using namespace el::text::literals;

using el::debug::DebugViewDetail;
using el::text::StringConverter;
using el::text::StringEditor;

TESTED_TARGETS(StringDebugBuilder)
class StringDebugBuilderTest final : public el::UnitTest {
public:
    void testStorageKindText() {

        REQUIRE_EQUAL(el::text::impl::toString(el::text::impl::StringStorageKind::Empty), "empty"_el);
        REQUIRE_EQUAL(el::text::impl::toString(el::text::impl::StringStorageKind::Shared), "shared"_el);
        REQUIRE_EQUAL(el::text::impl::toString(el::text::impl::StringStorageKind::Literal), "literal"_el);
    }

    void testStorageIdentifierText() {
        REQUIRE_EQUAL(el::text::impl::toString(el::text::impl::StringStorageKind::Empty), "empty"_el);
    }

    void testMakeStringDebugTree() {

        const auto text = StringEditor{"A\nB"_el};
        const auto details = DebugViewDetail::ContentInTitle | DebugViewDetail::CoreDetails;
        const auto output =
            StringConverter{el::debug::impl::makeStringDebugTree("StringEditor"_el, text, details).toString()}
                .toStdString();

        REQUIRE(containsText(output, "StringEditor(\"A\\nB\")"));
        REQUIRE(containsText(output, "isEncodingValid: true"));
        REQUIRE(containsText(output, "characterLength: 3"));
    }

private:
    [[nodiscard]] static auto containsText(const std::string &text, const std::string &needle) noexcept -> bool {
        return text.find(needle) != std::string::npos;
    }
};
