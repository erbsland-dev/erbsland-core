// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../ConfTestHelper.hpp"

#include <erbsland/conf/Document.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;
using namespace el::text::literals;

class ValueTestHelper : public ConfTestHelper {
public:
    DocumentPtr doc;
    ConstValuePtr value;

    auto additionalErrorMessages() -> std::string override {
        try {
            std::string result;
            const auto format =
                TestFormat(TestFormat::ShowContainerSize, TestFormat::ShowPosition, TestFormat::ShowSourceIdentifier);
            if (doc != nullptr) {
                result += "doc:\n";
                result += el::text::StringConverter{doc->toTestValueTree(format)}.toStdString();
            }
            if (value != nullptr) {
                result += "value:\n";
                result += el::text::StringConverter{value->toTestValueTree(format)}.toStdString();
            }
            return result;
        } catch (...) {
            return "Exception while creating additional error messages.";
        }
    }

    // Basic document that features all name path and section forms.
    constexpr static auto template1 = "[main]\n"
                                      "value1 = {0}\n"
                                      "value2 = {1}\n"
                                      "value3 = {2}\n"
                                      "nok_value = {3}\n"
                                      "value_list = {0}, {1}, {2}\n"
                                      "nok_value_list = {0}, {3}, {2}\n"
                                      "value_matrix =\n"
                                      "    * {0}, {1}, {2}\n"
                                      "    * {1}, {2}, {0}\n"
                                      "    * {2}, {0}, {1}\n"
                                      "nok_value_matrix =\n"
                                      "    * {0}, {1}, {2}\n"
                                      "    * {1}, {2}, {3}\n"
                                      "    * {2}, {0}, {1}\n"
                                      "[main.sub.sub.a]\n"
                                      "value = {0}\n"
                                      "[main.sub.sub.b]\n"
                                      "value = {1}\n"
                                      "[main.sub.sub.c]\n"
                                      "value = {2}\n"
                                      "*[list]\n"
                                      "value = {0}\n"
                                      "*[list]\n"
                                      "value = {1}\n"
                                      "*[list]\n"
                                      "value = {2}\n"
                                      "[main.text]\n"
                                      "\"first\" = {0}\n"
                                      "\"second\" = {1}\n"
                                      "\"third\" = {2}\n"
                                      "[main.sub_text.\"first\"]\n"
                                      "value = {0}\n"
                                      "[main.sub_text.\"second\"]\n"
                                      "value = {1}\n"
                                      "[main.sub_text.\"third\"]\n"
                                      "value = {2}\n"
                                      "# EOF\n";

    void setupTemplate1(
        std::string value1, std::string value2 = {}, std::string value3 = {}, std::string nok_value = {}) {

        if (value2.empty()) {
            value2 = value1;
        }
        if (value3.empty()) {
            value3 = value1;
        }
        if (nok_value.empty()) {
            nok_value = "false";
        }
        auto documentText = el::text::String{std::format(template1, value1, value2, value3, nok_value)};
        Parser parser;
        REQUIRE_NOTHROW(doc = parser.parseOrThrow(Source::fromString(documentText)));
    }

    constexpr static auto template2 = "[main]\nvalue: {}\n# EOF\n";

    void setupTemplate2(std::string valueText) {
        auto documentText = el::text::String{std::format(template2, valueText)};
        Parser parser;
        REQUIRE_NOTHROW(doc = parser.parseOrThrow(Source::fromString(documentText)));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main.value"}));
    }

    constexpr static auto template3 = "[main]\n"
                                      "value: {0}\n"
                                      "list: {0}, {0}, {0}\n"
                                      "[text]\n"
                                      "\"text\": {0}\n"
                                      "# EOF\n";

    void setupTemplate3(std::string valueText) {
        auto documentText = el::text::String{std::format(template3, valueText)};
        Parser parser;
        REQUIRE_NOTHROW(doc = parser.parseOrThrow(Source::fromString(documentText)));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main.value"}));
    }
};
