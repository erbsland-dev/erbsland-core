// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/impl/vr/Rule.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(Rule Rules RuleMap)
TAGS(ValidationRules)
class VrRulePathsTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    auto rootRule() -> impl::RulePtr {
        const auto implementation = std::dynamic_pointer_cast<impl::Rules>(rules);
        REQUIRE(implementation);
        return implementation->root();
    }

    void testRolesAndLookupIdentities() {
        WITH_CONTEXT(requireRulesPass(R"(
[app]
type: "section"
[app.vr_literal_type]
type: "text"
[app.vr_vr_entry]
type: "text"
[app.vr_literal_vr_name]
type: "integer"
[app.vr_name]
starts: "app"
[app.vr_any]
type: "boolean"
[items]
type: "section_list"
[items.vr_entry.vr_literal_type]
type: "text"
*[choice]
type: "text"
*[choice]
type: "integer"
)"_el));
        const auto root = rootRule();
        REQUIRE(root->role() == impl::RuleRole::Root);
        REQUIRE(root->definitionNamePath().empty());
        REQUIRE(root->valueNamePath().empty());
        const auto app = root->descendantByValuePath(NamePath::fromText("app"_el));
        REQUIRE(app);
        REQUIRE(app->role() == impl::RuleRole::NamedValue);
        const auto literal = app->childByValueName(Name::createRegular("type"_el));
        REQUIRE(literal);
        REQUIRE_EQUAL(literal, app->childByDefinitionName(Name::createRegular("vr_literal_type"_el)));
        REQUIRE_EQUAL(literal->definitionNamePath().toText(), "app.vr_literal_type"_el);
        REQUIRE_EQUAL(literal->valueNamePath().toText(), "app.type"_el);
        REQUIRE_EQUAL(literal->namePath(), literal->valueNamePath());
        REQUIRE(!app->childByDefinitionName(Name::createRegular("type"_el)));
        REQUIRE(!app->childByValueName(Name::createRegular("vr_literal_type"_el)));
        REQUIRE(app->nameConstraintRule()->role() == impl::RuleRole::NameConstraint);
        REQUIRE_EQUAL(app->nameConstraintRule()->namePath().toText(), "app.vr_name"_el);
        REQUIRE(app->childByValueName(Name::createRegular("vr_name"_el))->role() == impl::RuleRole::NamedValue);
        REQUIRE(app->anyValueRule()->role() == impl::RuleRole::AnyValue);
        REQUIRE(!app->listEntryRule());
        REQUIRE(app->childByValueName(Name::createRegular("vr_entry"_el))->role() == impl::RuleRole::NamedValue);
        const auto entry = root->descendantByValuePath(NamePath::fromText("items.vr_entry"_el));
        REQUIRE(entry);
        REQUIRE(entry->role() == impl::RuleRole::ListEntry);
        REQUIRE_EQUAL(entry->valueNamePath().toText(), "items.vr_entry"_el);
        REQUIRE(!entry->parent()->childByValueName(Name::createRegular("vr_entry"_el)));
        REQUIRE(root->descendantByValuePath(NamePath::fromText("items.vr_entry.type"_el)));
        const auto choice = root->descendantByValuePath(NamePath::fromText("choice"_el));
        REQUIRE(choice);
        const auto alternative = root->descendantByDefinitionPath(NamePath::fromText("choice[0]"_el));
        REQUIRE(alternative);
        REQUIRE(alternative->role() == impl::RuleRole::Alternative);
        REQUIRE_EQUAL(alternative->valueNamePath(), choice->valueNamePath());
        REQUIRE_EQUAL(alternative->definitionNamePath().toText(), "choice[0]"_el);
        REQUIRE(!root->descendantByValuePath(NamePath::fromText("choice.choice"_el)));
        REQUIRE(!root->descendantByValuePath(NamePath::fromText("choice[0]"_el)));
        REQUIRE(!root->descendantByValuePath(NamePath::fromText("items[0].type"_el)));
        REQUIRE(!app->descendantByValuePath(NamePath::fromText("vr_any"_el)));
        REQUIRE(!app->descendantByValuePath(NamePath::fromText("unknown"_el)));
        REQUIRE(!app->descendantByValuePath(NamePath::fromText("\"type\""_el)));
        REQUIRE(!root->descendantByValuePath(NamePath{}));
        REQUIRE(!root->descendantByDefinitionPath(NamePath{}));
        REQUIRE(!root->descendantByDefinitionPath(NamePath::fromText("choice[9]"_el)));
    }

    void testTemplatesAndNameConstraintParity() {
        WITH_CONTEXT(requireRulesPass(R"(
[vr_template.vr_literal_settings]
type: "section"
[vr_template.vr_literal_settings.vr_name]
type: "text"
[vr_template.vr_literal_settings.vr_vr_type]
type: "text"
[app.vr_literal_type]
use_template: "vr_literal_settings"
[app.vr_vr_type]
use_template: "vr_literal_settings"
)"_el));
        const auto root = rootRule();
        const auto first = root->descendantByValuePath(NamePath::fromText("app.type"_el));
        const auto second = root->descendantByValuePath(NamePath::fromText("app.vr_type"_el));
        REQUIRE(first);
        REQUIRE(second);
        REQUIRE(first != second);
        REQUIRE_EQUAL(first->nameConstraintRule()->namePath().toText(), "app.type.vr_name"_el);
        REQUIRE_EQUAL(first->nameConstraintRule()->definitionNamePath().toText(), "app.vr_literal_type.vr_name"_el);
        REQUIRE_EQUAL(
            second->childByValueName(Name::createRegular("vr_type"_el))->namePath().toText(), "app.vr_type.vr_type"_el);
        auto builder = vr::RulesBuilder{};
        builder.addRule("app"_el, vr::RuleType::Section);
        builder.addRule("app.type"_el, vr::RuleType::Section);
        builder.addRule("app.type.vr_name"_el, vr::RuleType::Text);
        rules = builder.takeRules();
        const auto built = rootRule()->descendantByValuePath(NamePath::fromText("app.type"_el));
        REQUIRE_EQUAL(built->nameConstraintRule()->namePath(), first->nameConstraintRule()->namePath());
    }

    void testBuilderAlternativesAndImplicitEntries() {
        using namespace vr::builder;
        auto builder = vr::RulesBuilder{};
        builder.addAlternative("vr_literal_type"_el, vr::RuleType::ValueList, EntryType(vr::RuleType::Text));
        builder.addAlternative("vr_literal_type"_el, vr::RuleType::Integer);
        rules = builder.takeRules();
        const auto root = rootRule();
        const auto branch = root->descendantByDefinitionPath(NamePath::fromText("vr_literal_type[0]"_el));
        REQUIRE(branch);
        REQUIRE(branch->role() == impl::RuleRole::Alternative);
        REQUIRE(branch->listEntryRule()->role() == impl::RuleRole::ListEntry);
        REQUIRE_EQUAL(branch->listEntryRule()->definitionNamePath().toText(), "vr_literal_type[0].vr_entry"_el);
        REQUIRE_EQUAL(branch->listEntryRule()->valueNamePath().toText(), "vr_literal_type.vr_entry"_el);
        REQUIRE(!root->descendantByValuePath(NamePath::fromText("type"_el)));
        REQUIRE(!root->descendantByValuePath(NamePath::fromText("vr_literal_type.vr_entry"_el)));
        REQUIRE(branch->descendantByValuePath(NamePath::fromText("vr_entry"_el)));
    }
};
