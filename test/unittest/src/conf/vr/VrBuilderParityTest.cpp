// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;
using namespace el::conf::vr::builder;
using namespace el::text::literals;

/// Custom public builder attribute used to verify the stable extension interface.
/// @tested{VrBuilderParityTest}
class CustomRootTitle final : public Attribute {
public:
    void apply(RuleDefinition &rule) const override { rule.setTitle("Parity Rules"_el); }
};

TESTED_TARGETS(RulesBuilder RuleDefinition DependencyMode Attribute)
TAGS(ValidationRules)
class VrBuilderParityTest final : public el::UnitTest {
private:
    [[nodiscard]] static auto createProgrammaticRules() -> el::conf::vr::RulesPtr {
        using namespace el::conf::vr::builder;
        auto builder = el::conf::vr::RulesBuilder{};
        builder.configureRoot(
            CustomRootTitle{},
            Description("Rules built through the public C++ API."_el),
            Dependency(
                el::conf::vr::DependencyMode::XNOR,
                {"certificate"_el},
                {"key"_el},
                "Certificate and key must be configured together."_el),
            KeyIndex("profile"_el, "profiles.vr_entry.id"_el));
        builder.addRule("certificate"_el, el::conf::vr::RuleType::Text, IsOptional());
        builder.addRule("key"_el, el::conf::vr::RuleType::Text, IsOptional());
        builder.addRule("profiles"_el, el::conf::vr::RuleType::SectionList, IsOptional());
        builder.addRule("profiles.vr_entry"_el, el::conf::vr::RuleType::Section);
        builder.addRule("profiles.vr_entry.id"_el, el::conf::vr::RuleType::Text);
        builder.addRule("selected"_el, el::conf::vr::RuleType::Text, IsOptional(), ConfKey("profile"_el));
        return builder.takeRules();
    }

    [[nodiscard]] static auto createDocumentRules() -> el::conf::vr::RulesPtr {
        const auto rulesDocument = Parser{}.parseTextOrThrow(
            "*[vr_dependency]*\n"
            "mode: \"xnor\"\n"
            "source: \"certificate\"\n"
            "target: \"key\"\n"
            "error: \"Certificate and key must be configured together.\"\n"
            "*[vr_key]*\n"
            "name: \"profile\"\n"
            "key: \"profiles.vr_entry.id\"\n"
            "[certificate]\n"
            "type: \"text\"\n"
            "is_optional: yes\n"
            "[key]\n"
            "type: \"text\"\n"
            "is_optional: yes\n"
            "[profiles]\n"
            "type: \"section_list\"\n"
            "is_optional: yes\n"
            "[profiles.vr_entry]\n"
            "type: \"section\"\n"
            "[profiles.vr_entry.id]\n"
            "type: \"text\"\n"
            "[selected]\n"
            "type: \"text\"\n"
            "is_optional: yes\n"
            "key: \"profile\"\n"_el);
        return el::conf::vr::Rules::createFromDocument(rulesDocument);
    }

    [[nodiscard]] static auto validates(const el::conf::vr::RulesPtr &rules, const el::text::String &documentText)
        -> bool {
        try {
            const auto document = Parser{}.parseTextOrThrow(documentText);
            rules->validate(document->valueOrThrow("profile"_el), 1);
            return true;
        } catch (const ConfError &) {
            return false;
        }
    }

public:
    void testRootMetadataUsesPublicAttributeInterface() {
        const auto rules = createProgrammaticRules();
        const auto document = Parser{}.parseTextOrThrow(""_el);

        REQUIRE_NOTHROW(rules->validate(document, 1));
        REQUIRE(document->validationRule());
        REQUIRE_EQUAL(document->validationRule()->title(), "Parity Rules"_el);
        REQUIRE_EQUAL(document->validationRule()->description(), "Rules built through the public C++ API."_el);
    }

    void testProgrammaticAndDocumentRulesHaveBehavioralParity() {
        const auto programmaticRules = createProgrammaticRules();
        const auto documentRules = createDocumentRules();
        const auto cases = std::vector<std::pair<el::text::String, bool>>{
            {"[profile]\n"_el, true},
            {"[profile]\ncertificate: \"cert\"\nkey: \"key\"\n"_el, true},
            {"[profile]\ncertificate: \"cert\"\n"_el, false},
            {"[profile]\nkey: \"key\"\n"_el, false},
            {"[profile]\nselected: \"primary\"\n*[profile.profiles]*\nid: \"primary\"\n"_el, true},
            {"[profile]\nselected: \"missing\"\n*[profile.profiles]*\nid: \"primary\"\n"_el, false},
            {"*[profile.profiles]*\nid: \"same\"\n*[profile.profiles]*\nid: \"same\"\n"_el, false},
        };

        for (const auto &[documentText, expected] : cases) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    REQUIRE_EQUAL(validates(programmaticRules, documentText), expected);
                    REQUIRE_EQUAL(validates(documentRules, documentText), expected);
                },
                [&]() -> std::string { return el::text::StringConverter{documentText}.toStdString(); });
        }
    }
};
