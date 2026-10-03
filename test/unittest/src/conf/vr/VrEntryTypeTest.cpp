// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all_conf_vr.hpp>
#include <erbsland/all_conf_vr_builder.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/impl/vr/Rule.hpp>
#include <erbsland/conf/impl/vr/Rules.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
using namespace el::conf::vr;
using namespace el::conf::vr::builder;

TESTED_TARGETS(EntryType EntryConstraints EntryTitle EntryDescription RulesBuilder RuleDefinition)
class VrEntryTypeTest final : public el::UnitTest {
private:
    class CustomMinimum final : public ConstraintAttribute {
    public:
        void apply(RuleDefinition &rule) const override { Minimum{2}.apply(rule); }
    };

    static constexpr auto cDataValue = "data"_el;
    static constexpr auto cSampleValue = "sample"_el;
    static constexpr auto cSampleValuesValue = "sample.values"_el;
    static constexpr auto cSampleValuesVrEntryValue = "sample.values.vr_entry"_el;

public:
    void testExplicitEntryParity() {
        for (const auto implicit : {false, true}) {
            auto builder = RulesBuilder{};
            builder.addRule(cSampleValue, RuleType::Section);
            if (implicit) {
                builder.addRule(cSampleValuesValue, RuleType::ValueList, EntryType{RuleType::Text}, IsOptional{});
            } else {
                builder.addRule(cSampleValuesValue, RuleType::ValueList, IsOptional{});
                builder.addRule(cSampleValuesVrEntryValue, RuleType::Text);
            }
            const auto rules = builder.takeRules();
            for (
                const auto &text :
                {"[sample]\n"_el, "[sample]\nvalues: \"one\"\n"_el, "[sample]\nvalues: \"one\", \"two\"\n"_el}) {
                const auto document = el::conf::Parser{}.parseTextOrThrow(text);
                REQUIRE_NOTHROW(rules->validate(document, 1));
            }
            for (const auto &text : {"[sample]\nvalues: 42\n"_el, "[sample]\nvalues: \"one\", 42\n"_el}) {
                const auto document = el::conf::Parser{}.parseTextOrThrow(text);
                REQUIRE_THROWS_AS(el::conf::ConfError, rules->validate(document, 1));
            }
        }
    }

    void testAlternativesKeepIndependentEntryTypes() {
        auto builder = RulesBuilder{};
        builder.addRule(cSampleValue, RuleType::Section);
        builder.addAlternative(cSampleValuesValue, RuleType::ValueList, EntryType{RuleType::Integer});
        builder.addAlternative(cSampleValuesValue, RuleType::ValueList, EntryType{RuleType::Text});
        const auto rules = builder.takeRules();
        for (const auto &text : {"[sample]\nvalues: 1, 2\n"_el, "[sample]\nvalues: \"one\", \"two\"\n"_el}) {
            const auto document = el::conf::Parser{}.parseTextOrThrow(text);
            REQUIRE_NOTHROW(rules->validate(document, 1));
        }
        const auto mixed = el::conf::Parser{}.parseTextOrThrow("[sample]\nvalues: 1, \"two\"\n"_el);
        REQUIRE_THROWS_AS(el::conf::ConfError, rules->validate(mixed, 1));
    }

    void testInvalidAttributesAndDuplicateEntries() {
        auto builder = RulesBuilder{};
        builder.addRule(cSampleValue, RuleType::Section);
        REQUIRE_THROWS_AS(
            el::conf::ConfError, builder.addRule("sample.value"_el, RuleType::Text, EntryType{RuleType::Text}));
        for (const auto type : {RuleType::Undefined, RuleType::Section, RuleType::ValueList, RuleType::Alternatives}) {
            REQUIRE_THROWS_AS(
                el::conf::ConfError, builder.addRule(cSampleValuesValue, RuleType::ValueList, EntryType{type}));
        }
        REQUIRE_THROWS_AS(
            el::conf::ConfError,
            builder.addRule(
                cSampleValuesValue, RuleType::ValueList, EntryType{RuleType::Text}, EntryType{RuleType::Integer}));
        REQUIRE_THROWS_AS(el::conf::ConfError, builder.configureRoot(EntryType{RuleType::Text}));
        builder.addRule(cSampleValuesValue, RuleType::ValueList, EntryType{RuleType::Text});
        REQUIRE_THROWS_AS(el::conf::ConfError, builder.addRule(cSampleValuesVrEntryValue, RuleType::Text));
        REQUIRE_NOTHROW(builder.takeRules());
    }

    void testAnyScalarAndBuilderReuse() {
        auto builder = RulesBuilder{};
        for (const auto type : {RuleType::Value, RuleType::Boolean}) {
            builder.addRule(cSampleValue, RuleType::Section);
            builder.addRule(cSampleValuesValue, RuleType::ValueList, EntryType{type});
            const auto rules = builder.takeRules();
            const auto document = el::conf::Parser{}.parseTextOrThrow("[sample]\nvalues: Yes, No\n"_el);
            REQUIRE_NOTHROW(rules->validate(document, 1));
            const auto mixed = el::conf::Parser{}.parseTextOrThrow("[sample]\nvalues: Yes, 42\n"_el);
            if (type == RuleType::Value) {
                REQUIRE_NOTHROW(rules->validate(mixed, 1));
            } else {
                REQUIRE_THROWS_AS(el::conf::ConfError, rules->validate(mixed, 1));
            }
        }
    }
    void testConstrainedEntryParityAndMetadata() {
        for (const auto implicit : {false, true}) {
            WITH_CONTEXT(runWithContext(SOURCE_LOCATION(), [&]() -> void {
                RulesBuilder builder;
                builder.addRule(cSampleValue, RuleType::Section);
                if (implicit) {
                    builder.addRule(
                        cSampleValuesValue,
                        RuleType::ValueList,
                        EntryTitle{"Entry"_el},
                        EntryDescription{"Entry description"_el},
                        EntryConstraints{Minimum{2}, Matches{R"(\A[a-z]+\z)"_el}},
                        EntryType{RuleType::Text},
                        Title{"List"_el},
                        IsOptional{});
                } else {
                    builder.addRule(cSampleValuesValue, RuleType::ValueList, Title{"List"_el}, IsOptional{});
                    builder.addRule(
                        cSampleValuesVrEntryValue,
                        RuleType::Text,
                        Title{"Entry"_el},
                        Description{"Entry description"_el},
                        Minimum{2},
                        Matches{R"(\A[a-z]+\z)"_el});
                }
                const auto rules = std::dynamic_pointer_cast<el::conf::impl::Rules>(builder.takeRules());
                const auto list = rules->ruleForNamePath(el::conf::NamePath::fromText(cSampleValuesValue));
                const auto entry = list->child(el::conf::Name::vrName(el::conf::Name::VR::ReservedEntry));
                REQUIRE_EQUAL(list->title(), "List"_el);
                REQUIRE(list->isOptional());
                REQUIRE(list->constraints().empty());
                REQUIRE_EQUAL(entry->title(), "Entry"_el);
                REQUIRE_EQUAL(entry->description(), "Entry description"_el);
                REQUIRE_FALSE(entry->isOptional());
                REQUIRE_EQUAL(entry->constraints().size(), 2U);
                for (
                    const auto &text :
                    {"[sample]\n"_el, "[sample]\nvalues: \"ab\"\n"_el, "[sample]\nvalues: \"ab\", \"cd\"\n"_el}) {
                    const auto document = el::conf::Parser{}.parseTextOrThrow(text);
                    REQUIRE_NOTHROW(rules->validate(document, 1));
                }
                for (
                    const auto &text :
                    {"[sample]\nvalues: \"a\"\n"_el,
                        "[sample]\nvalues: \"ab\", \"12\"\n"_el,
                        "[sample]\nvalues: 42\n"_el}) {
                    const auto document = el::conf::Parser{}.parseTextOrThrow(text);
                    REQUIRE_THROWS_AS(el::conf::ConfError, rules->validate(document, 1));
                }
            }));
        }
    }

    void testStoredAttributesCustomConstraintsAndReplacement() {
        const auto constraints = EntryConstraints{CustomMinimum{}, Maximum{4}};
        const auto title = EntryTitle{el::String{"Owned title"_el}};
        const auto description = EntryDescription{el::String{"Owned description"_el}};
        RulesBuilder builder;
        for (const auto &name : {"first"_el, "second"_el}) {
            builder.addRule(
                name,
                RuleType::ValueList,
                constraints,
                title,
                description,
                EntryType{RuleType::Text},
                EntryConstraints{Minimum{3}},
                EntryTitle{"Final title"_el},
                EntryDescription{"Final description"_el});
        }
        builder.addRule("empty"_el, RuleType::ValueList, EntryConstraints{}, EntryType{RuleType::Text}, IsOptional{});
        const auto rules = std::dynamic_pointer_cast<el::conf::impl::Rules>(builder.takeRules());
        for (const auto &name : {"first"_el, "second"_el}) {
            const auto list = rules->ruleForNamePath(el::conf::NamePath::fromText(name));
            const auto entry = list->child(el::conf::Name::vrName(el::conf::Name::VR::ReservedEntry));
            REQUIRE_EQUAL(entry->title(), "Final title"_el);
            REQUIRE_EQUAL(entry->description(), "Final description"_el);
            REQUIRE_EQUAL(entry->constraints().size(), 2U);
        }
        // ELCL settings belong to sections, so validate the root schema against a section branch.
        const auto valid = el::conf::Parser{}.parseTextOrThrow("[data]\nfirst: \"abc\"\nsecond: \"abcd\"\n"_el);
        REQUIRE_NOTHROW(rules->validate(valid->value(cDataValue), 1));
        const auto invalid = el::conf::Parser{}.parseTextOrThrow("[data]\nfirst: \"ab\"\nsecond: \"abcd\"\n"_el);
        REQUIRE_THROWS_AS(el::conf::ConfError, rules->validate(invalid->value(cDataValue), 1));
    }

    void testEntryErrorsDoNotAttachRulesOrAlternatives() {
        RulesBuilder builder;
        builder.addRule(cSampleValue, RuleType::Section);
        REQUIRE_THROWS_AS(
            el::conf::ConfError,
            builder.addRule(cSampleValuesValue, RuleType::ValueList, EntryConstraints{Minimum{1}}));
        REQUIRE_THROWS_AS(
            el::conf::ConfError, builder.addRule(cSampleValuesValue, RuleType::ValueList, EntryTitle{"Entry"_el}));
        REQUIRE_THROWS_AS(
            el::conf::ConfError,
            builder.addRule(cSampleValuesValue, RuleType::ValueList, EntryDescription{"Entry"_el}));
        REQUIRE_THROWS_AS(
            el::conf::ConfError, builder.addRule(cSampleValuesValue, RuleType::ValueList, EntryConstraints{}));
        REQUIRE_THROWS_AS(
            el::conf::ConfError, builder.addRule(cSampleValuesValue, RuleType::Text, EntryConstraints{Minimum{1}}));
        REQUIRE_THROWS_AS(
            el::conf::ConfError,
            builder.addRule(
                cSampleValuesValue,
                RuleType::ValueList,
                EntryConstraints{Matches{"a"_el}},
                EntryType{RuleType::Integer}));
        REQUIRE_THROWS_AS(
            el::conf::ConfError,
            builder.addAlternative(
                cSampleValuesValue,
                RuleType::ValueList,
                EntryType{RuleType::Integer},
                EntryConstraints{Matches{"a"_el}}));
        // Neither failed parent nor failed alternative may occupy the path.
        builder.addRule(
            cSampleValuesValue, RuleType::ValueList, EntryType{RuleType::Text}, EntryConstraints{Minimum{1}});
        auto branch = builder.createBranch(cSampleValuesValue);
        REQUIRE_THROWS_AS(el::conf::ConfError, branch.configureRoot(EntryTitle{"Missing type"_el}));
        REQUIRE_THROWS_AS(el::conf::ConfError, branch.configureRoot(EntryType{RuleType::Text}));
        REQUIRE_THROWS_AS(el::conf::ConfError, builder.addRule(cSampleValuesVrEntryValue, RuleType::Text));
        REQUIRE_NOTHROW(builder.takeRules());
    }

    void testBranchConfigurationCanCreateAnImplicitEntry() {
        RulesBuilder builder;
        builder.addRule(cSampleValue, RuleType::Section);
        builder.addRule(cSampleValuesValue, RuleType::ValueList);
        auto branch = builder.createBranch(cSampleValuesValue);
        branch.configureRoot(EntryConstraints{Minimum{1}}, EntryTitle{"Configured"_el}, EntryType{RuleType::Text});
        const auto rules = builder.takeRules();
        const auto document = el::conf::Parser{}.parseTextOrThrow("[sample]\nvalues: \"ok\"\n"_el);
        REQUIRE_NOTHROW(rules->validate(document, 1));
    }
};
