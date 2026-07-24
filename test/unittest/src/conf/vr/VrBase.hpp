// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/vr/Rules.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/err/DiagnosticHelper.hpp>
#include <erbsland/text/CaseSensitivity.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/TextDocument.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;
using namespace el::text::literals;
using el::text::CaseSensitivity;

class VrBase : public el::UnitTest {
public:
    el::text::String failedText;
    DocumentPtr vrDocument;
    vr::RulesPtr rules;
    DocumentPtr document;
    el::text::String lastError;

    void setLastError(const ConfError &error) { lastError = el::err::DiagnosticHelper{error}.toDocument().toString(); }

    void setUp() override {
        failedText = {};
        vrDocument = nullptr;
        rules = nullptr;
        document = nullptr;
        lastError = {};
    }

    auto additionalErrorMessages() -> std::string override {
        try {
            std::string result;
            if (!failedText.isEmpty()) {
                result += std::format("Failed text:\n{}\n", el::text::StringConverter{failedText}.toStdString());
            }
            if (vrDocument == nullptr) {
                result += "VR document: <null>\n";
            } else {
                result += std::format(
                    "VR document:\n{}\n", el::text::StringConverter{vrDocument->toTestValueTree()}.toStdString());
            }
            auto rulesImpl = std::dynamic_pointer_cast<impl::Rules>(rules);
            if (rulesImpl == nullptr) {
                result += "Validated rules: <null>\n";
            } else {
                result += std::format(
                    "Validated rules:\n{}\n",
                    el::text::StringConverter{internalView(rulesImpl)->toString()}.toStdString());
            }
            if (document == nullptr) {
                result += "Validated document: <null>\n";
            } else {
                result += std::format(
                    "Validated document:\n{}\n", el::text::StringConverter{document->toTestValueTree()}.toStdString());
            }
            if (lastError.isEmpty()) {
                result += "Last error: <empty>\n";
            } else {
                result += std::format("Last error: {}\n", el::text::StringConverter{lastError}.toStdString());
            }
            return result;
        } catch (...) {
            return "Unexpected exception thrown";
        }
    }

    [[nodiscard]] static auto linesToString(const std::vector<std::string_view> &lines) -> el::text::String {
        el::text::StringEditor result;
        for (const auto &line : lines) {
            result.append(el::text::String{line});
            result.append(el::text::String{"\n"});
        }
        return result;
    }

    void requireRulesPass(const el::text::String &text) {
        lastError = {};
        try {
            Parser vrParser;
            vrDocument = vrParser.parseTextOrThrow(text);
        } catch (const ConfError &e) {
            failedText = text;
            setLastError(e);
            REQUIRE(false);
        }
        REQUIRE(vrDocument != nullptr);
        try {
            rules = vr::Rules::createFromDocument(vrDocument);
        } catch (const ConfError &e) {
            setLastError(e);
            REQUIRE(false);
        }
        REQUIRE(rules != nullptr);
    }

    void requireRulesPass(const std::string_view text) { requireRulesPass(el::text::String{text}); }

    void requireRulesPass(const std::u8string_view text) { requireRulesPass(el::text::String{text}); }

    void requireRulesPassLines(const std::vector<std::string_view> &lines) { requireRulesPass(linesToString(lines)); }

    /// Test if compiling *rules* fail. Expects a valid configuration document.
    void requireRulesFail(const el::text::String &text) {
        lastError = {};
        try {
            Parser vrParser;
            vrDocument = vrParser.parseTextOrThrow(text);
        } catch (const ConfError &e) {
            failedText = text;
            setLastError(e);
            REQUIRE(false);
        }
        REQUIRE(vrDocument != nullptr);
        try {
            rules = vr::Rules::createFromDocument(vrDocument);
            REQUIRE(false);
        } catch (const ConfError &e) {
            setLastError(e);
        }
    }

    void requireRulesFail(const std::string_view text) { requireRulesFail(el::text::String{text}); }

    void requireRulesFail(const std::u8string_view text) { requireRulesFail(el::text::String{text}); }

    /// Test if compiling *rules* fail. Expects a valid configuration document.
    void requireRulesFailLines(const std::vector<std::string_view> &lines) { requireRulesFail(linesToString(lines)); }

    void requirePass(const el::text::String &text, const Integer version = 0) {
        lastError = {};
        Parser docParser;
        try {
            document = docParser.parseTextOrThrow(text);
        } catch (const ConfError &e) {
            setLastError(e);
            REQUIRE(false);
        }
        REQUIRE(document != nullptr);
        REQUIRE(rules != nullptr);
        try {
            rules->validate(document, version);
        } catch (const ConfError &e) {
            setLastError(e);
            REQUIRE(false);
        }
    }

    void requirePass(const std::string_view text, const Integer version = 0) {
        requirePass(el::text::String{text}, version);
    }

    void requirePass(const std::u8string_view text, const Integer version = 0) {
        requirePass(el::text::String{text}, version);
    }

    void requirePassLines(const std::vector<std::string_view> &lines, const Integer version = 0) {
        requirePass(linesToString(lines), version);
    }

    void requireFail(const el::text::String &text, const Integer version = 0) {
        lastError = {};
        Parser docParser;
        REQUIRE_NOTHROW(document = docParser.parseTextOrThrow(text));
        REQUIRE(document != nullptr);
        REQUIRE(rules != nullptr);
        try {
            rules->validate(document, version);
            REQUIRE(false);
        } catch (const ConfError &e) {
            REQUIRE_EQUAL(e.category(), ConfErrorCategory::Validation);
            setLastError(e);
        }
    }

    void requireFail(const std::string_view text, const Integer version = 0) {
        requireFail(el::text::String{text}, version);
    }

    void requireFail(const std::u8string_view text, const Integer version = 0) {
        requireFail(el::text::String{text}, version);
    }

    void requireFailLines(const std::vector<std::string_view> &lines, const Integer version = 0) {
        requireFail(linesToString(lines), version);
    }

    void requireError(const std::string_view partialMatch) {
        REQUIRE(lastError.contains(el::text::String{partialMatch}, el::text::cCaseInsensitive.asciiComparisonFn()));
    }

    void requireError(const std::u8string_view partialMatch) {
        REQUIRE(lastError.contains(el::text::String{partialMatch}, el::text::cCaseInsensitive.asciiComparisonFn()));
    }

    void requireError(const el::text::String &partialMatch) {
        REQUIRE(lastError.contains(partialMatch, el::text::cCaseInsensitive.asciiComparisonFn()));
    }

    [[nodiscard]] static auto buildOneConstraintDoc(
        const std::string &constraintLine, const vr::RuleType ruleType, bool caseSensitive = false)
        -> el::text::String {
        auto typeLine = std::format("type: \"{}\"", el::text::StringConverter{ruleType.toText()}.toStdString());
        std::vector<std::string_view> lines = {
            "[app.x]",
            std::string_view{typeLine},
            std::string_view{constraintLine},
        };
        if (ruleType == vr::RuleType::ValueList || ruleType == vr::RuleType::ValueMatrix) {
            lines.emplace_back("[app.x.vr_entry]");
            lines.emplace_back("type: \"integer\"");
        } else if (ruleType == vr::RuleType::SectionList) {
            lines.emplace_back("[app.x.vr_entry.y]");
            lines.emplace_back("type: \"integer\"");
        } else if (ruleType == vr::RuleType::Section) {
            lines.emplace_back("[app.x.vr_any]");
            lines.emplace_back("type: \"integer\"");
        } else if (ruleType == vr::RuleType::SectionWithTexts) {
            lines.emplace_back("[app.x.vr_any]");
            lines.emplace_back("type: \"integer\"");
        }
        if (caseSensitive) {
            lines.emplace_back("case_sensitive: true");
        }
        return linesToString(lines);
    }

    void requireOneConstraintPass(
        const std::string &constraintLine, const vr::RuleType ruleType, bool caseSensitive = false) {

        WITH_CONTEXT(requireRulesPass(buildOneConstraintDoc(constraintLine, ruleType, caseSensitive)))
    }

    void requireOneConstraintFail(
        const std::string &constraintLine, const vr::RuleType ruleType, bool caseSensitive = false) {

        WITH_CONTEXT(requireRulesFail(buildOneConstraintDoc(constraintLine, ruleType, caseSensitive)))
    }

    void requireConstraintValidForRuleTypes(
        const std::string &constraintLine, const std::set<vr::RuleType> &validRuleTypes) {

        for (auto testedRuleType : vr::RuleType::all()) {
            if (testedRuleType == vr::RuleType::Alternatives) {
                continue; // alternatives must be defined as a section list and do not allow constraints.
            }
            if (validRuleTypes.contains(testedRuleType)) {
                WITH_CONTEXT(requireOneConstraintPass(constraintLine, testedRuleType));
            } else {
                WITH_CONTEXT(requireOneConstraintFail(constraintLine, testedRuleType));
            }
        }
    }
};
