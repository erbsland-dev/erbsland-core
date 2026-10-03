// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all_conf_vr.hpp>
#include <erbsland/all_conf_vr_builder.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/impl/vr/Rule.hpp>
#include <erbsland/conf/impl/vr/Rules.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <type_traits>

using namespace el::text::literals;
using namespace el::conf;
using namespace el::conf::vr;
using namespace el::conf::vr::builder;

static_assert(!std::is_copy_constructible_v<RulesBuilderBranch>);
static_assert(!std::is_move_constructible_v<RulesBuilderBranch>);
static_assert(std::has_virtual_destructor_v<RulesBuilder>);

TESTED_TARGETS(RulesBuilder RulesBuilderBranch)
class VrBuilderBranchTest final : public el::UnitTest {
private:
    static constexpr auto cAppValue = "app"_el;
    static constexpr auto cEntriesValue = "entries"_el;
    static constexpr auto cValueName = "value"_el;

    static void addPort(RulesBuilder &builder) {
        builder.addRule("port"_el, RuleType::Integer, Minimum{1}, Maximum{65535});
    }

    auto finish(RulesBuilder &builder) -> std::shared_ptr<el::conf::impl::Rules> {
        RulesPtr finalized;
        try {
            finalized = builder.takeRules();
        } catch (const ConfError &error) {
            consoleWriteLine(el::StringConverter{error.description()}.toStdString());
            throw;
        }
        auto rules = std::dynamic_pointer_cast<el::conf::impl::Rules>(finalized);
        REQUIRE(rules);
        return rules;
    }

public:
    void testNestedBranchesAndPathForms() {
        RulesBuilder builder;
        REQUIRE(builder.parentPath().empty());
        auto app = builder.createBranch(Name::createRegular(cAppValue));
        REQUIRE_EQUAL(app.parentPath().toText(), cAppValue);
        REQUIRE_THROWS_AS(ConfError, app.addRule(cValueName, RuleType::Text));
        builder.addRule(cAppValue, RuleType::Section);
        app.addRule(NamePath::fromText("server"_el), RuleType::Section);
        auto server = app.createBranch(NamePath::fromText("server"_el));
        REQUIRE_EQUAL(server.parentPath().toText(), "app.server"_el);
        addPort(server);
        server.addRule(Name::createRegular("name"_el), RuleType::Text);
        app.addRule(cEntriesValue, RuleType::SectionList);
        auto entries = app.createBranch(cEntriesValue);
        entries.addRule("vr_entry"_el, RuleType::Section);
        auto entry = entries.createBranch(Name::vrName(Name::VR::ReservedEntry));
        entry.addRule(cValueName, RuleType::Text);
        app.addRule("extra"_el, RuleType::Section);
        auto extra = app.createBranch("extra"_el);
        extra.addRule("vr_any"_el, RuleType::Text);
        const auto rules = finish(builder);
        REQUIRE(rules->ruleForNamePath(NamePath::fromText("app.server.port"_el)));
        REQUIRE(rules->ruleForNamePath(NamePath::fromText("app.entries.vr_entry.value"_el)));
        REQUIRE(rules->ruleForNamePath(NamePath::fromText("app.extra.vr_any"_el)));
    }

    void testBranchPathsAreValidatedBeforePrefixing() {
        RulesBuilder builder;
        auto branch = builder.createBranch(cAppValue);
        builder.addRule(cAppValue, RuleType::Section);
        for (
            const auto &path :
            {NamePathLike{std::size_t{0}},
                NamePathLike{NamePath{}},
                NamePathLike{el::String{}},
                NamePathLike{el::String{"[0]"_el}},
                NamePathLike{el::String{"\"text\""_el}}}) {
            WITH_CONTEXT(runWithContext(SOURCE_LOCATION(), [&]() -> void {
                REQUIRE_THROWS_AS(ConfError, builder.createBranch(path));
                REQUIRE_THROWS_AS(ConfError, branch.createBranch(path));
                REQUIRE_THROWS_AS(ConfError, branch.addRule(path, RuleType::Text));
                REQUIRE_THROWS_AS(ConfError, branch.addAlternative(path, RuleType::Text));
            }));
        }
        REQUIRE_NOTHROW(builder.takeRules());
    }

    void testConfigureBranchParentAndRelativeRelations() {
        RulesBuilder builder;
        builder.configureRoot(Title{"Document"_el});
        auto branch = builder.createBranch(cAppValue);
        REQUIRE_THROWS_AS(ConfError, branch.configureRoot(Title{"Missing"_el}));
        builder.addRule(cAppValue, RuleType::Section);
        RulesBuilder &base = branch;
        base.configureRoot(
            Title{"Application"_el},
            Description{"Settings"_el},
            Dependency{DependencyMode::XNOR, {"left"_el}, {"right"_el}},
            KeyIndex("ids"_el, {"entries.vr_entry.id"_el}));
        branch.addRule("left"_el, RuleType::Text, IsOptional{});
        branch.addRule("right"_el, RuleType::Text, IsOptional{});
        branch.addRule(cEntriesValue, RuleType::SectionList, IsOptional{});
        branch.addRule("entries.vr_entry"_el, RuleType::Section);
        branch.addRule("entries.vr_entry.id"_el, RuleType::Text);
        const auto rules = finish(builder);
        REQUIRE_EQUAL(rules->root()->title(), "Document"_el);
        const auto app = rules->ruleForNamePath(NamePath::fromText(cAppValue));
        REQUIRE_EQUAL(app->title(), "Application"_el);
        REQUIRE_EQUAL(app->description(), "Settings"_el);
        REQUIRE(app->hasKeyDefinitions());
        REQUIRE_EQUAL(app->keyDefinitions().size(), 1U);
        auto valid = Parser{}.parseTextOrThrow("[app]\nleft: \"x\"\nright: \"y\"\n"_el);
        REQUIRE_NOTHROW(rules->validate(valid, 1));
        auto invalid = Parser{}.parseTextOrThrow("[app]\nleft: \"x\"\n"_el);
        REQUIRE_THROWS_AS(ConfError, rules->validate(invalid, 1));
    }

    void testBranchAlternativesAndEntryPaths() {
        RulesBuilder builder;
        builder.addRule(cAppValue, RuleType::Section);
        auto branch = builder.createBranch(cAppValue);
        RulesBuilder &base = branch;
        base.addAlternative(
            "values"_el,
            RuleType::ValueList,
            EntryConstraints{Minimum{1}},
            EntryType{RuleType::Integer},
            EntryTitle{"Number"_el});
        branch.addAlternative(
            "values"_el, RuleType::ValueList, EntryType{RuleType::Text}, EntryConstraints{Matches{"[a-z]+"_el}});
        const auto rules = finish(builder);
        const auto alternatives = rules->ruleForNamePath(NamePath::fromText("app.values"_el));
        REQUIRE_EQUAL(alternatives->type(), RuleType::Alternatives);
        REQUIRE_EQUAL(alternatives->children().size(), 2U);
        for (const auto &branch : alternatives->childrenImpl()) {
            const auto entry = branch->child(Name::vrName(Name::VR::ReservedEntry));
            REQUIRE(entry);
            REQUIRE(entry->parent() == branch);
            auto declaration = branch->ruleNamePath();
            declaration.append(Name::vrName(Name::VR::ReservedEntry));
            REQUIRE_EQUAL(entry->ruleNamePath(), declaration);
            REQUIRE_EQUAL(entry->targetNamePath().toText(), "app.values.vr_entry"_el);
        }
        for (const auto &text : {"[app]\nvalues: 1, 2\n"_el, "[app]\nvalues: \"one\", \"two\"\n"_el}) {
            const auto document = Parser{}.parseTextOrThrow(text);
            REQUIRE_NOTHROW(rules->validate(document, 1));
        }
        const auto document = Parser{}.parseTextOrThrow("[app]\nvalues: 0, -1\n"_el);
        REQUIRE_THROWS_AS(ConfError, rules->validate(document, 1));
    }

    void testBranchFollowsResetAndTakeRules() {
        RulesBuilder builder;
        auto branch = builder.createBranch(cAppValue);
        for (const auto reset : {false, true, false}) {
            builder.addRule(cAppValue, RuleType::Section);
            branch.configureRoot(Title{"Reused"_el});
            addPort(branch);
            if (reset) {
                builder.reset();
            } else {
                const auto rules = finish(builder);
                REQUIRE(rules->ruleForNamePath(NamePath::fromText("app.port"_el)));
            }
            REQUIRE_THROWS_AS(ConfError, branch.configureRoot(Title{"Missing"_el}));
            REQUIRE_THROWS_AS(ConfError, addPort(branch));
        }
    }

    void testBranchesOutliveTheirCreatorsAndRejectDocumentOperations() {
        auto owner = std::make_unique<RulesBuilder>();
        owner->addRule(cAppValue, RuleType::Section);
        auto branch = owner->createBranch(cAppValue);
        auto nested = [&branch]() -> RulesBuilderBranch {
            auto temporary = branch.createBranch("server"_el);
            branch.addRule("server"_el, RuleType::Section);
            return temporary.createBranch("deeper"_el);
        }();
        owner.reset();
        branch.configureRoot(Title{"Still alive"_el});
        branch.addRule("server.deeper"_el, RuleType::Section);
        REQUIRE_NOTHROW(addPort(nested));
        RulesBuilder &base = branch;
        REQUIRE_THROWS_AS(el::err::LogicFailure, base.reset());
        REQUIRE_THROWS_AS(el::err::LogicFailure, base.takeRules());
        REQUIRE_THROWS_AS(el::err::LogicFailure, nested.reset());
        REQUIRE_THROWS_AS(el::err::LogicFailure, nested.takeRules());
        REQUIRE_NOTHROW(branch.addRule("name"_el, RuleType::Text));
    }
};
