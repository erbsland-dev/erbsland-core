// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/impl/vr/Rule.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/StringFormat.hpp>

#include <array>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(Rules KeyDefinition)
TAGS(ValidationRules)
class VrLiteralPathsTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testCompositeIndexesWithLiteralControlNames() {
        WITH_CONTEXT(requireRulesPass(R"(
*[vr_key]
name: "vr_literal_index"
key: "vr_entry.vr_entry.type", "vr_entry.vr_entry.vr_entry.vr_type"
case_sensitive: Yes
[vr_vr_entry]
type: "section_list"
[vr_vr_entry.vr_entry.vr_literal_type]
type: "text"
[vr_vr_entry.vr_entry.vr_literal_vr_entry.vr_vr_type]
type: "integer"
[app.vr_literal_vr_key]
type: "text"
key: "vr_literal_index"
[app.vr_literal_type]
type: "text"
key: "vr_literal_index[0]"
[app.vr_vr_type]
type: "integer"
key: "vr_literal_index[1]"
)"_el));
        const auto implementation = std::dynamic_pointer_cast<impl::Rules>(rules);
        const auto definition = implementation->root()->keyDefinitions().front();
        REQUIRE_EQUAL(definition->listPath().toText(), "vr_entry"_el);
        REQUIRE_EQUAL(definition->entryValuePaths().at(0).toText(), "type"_el);
        REQUIRE_EQUAL(definition->entryValuePaths().at(1).toText(), "vr_entry.vr_type"_el);
        REQUIRE_EQUAL(definition->keys().at(1).toText(), "vr_entry.vr_entry.vr_entry.vr_type"_el);
        const auto valid = R"(
*[vr_entry]
type: "Alpha"
[.vr_entry]
vr_type: 7
[app]
vr_key: "Alpha,7"
type: "Alpha"
vr_type: 7
)"_el;
        WITH_CONTEXT(requirePass(valid));
        WITH_CONTEXT(requireFail(R"(
*[vr_entry]
type: "Alpha"
[.vr_entry]
vr_type: 7
[app]
vr_key: "Alpha,7"
type: "alpha"
vr_type: 7
)"_el));
        WITH_CONTEXT(requireError("existing key"));
        WITH_CONTEXT(requireFail(R"(
*[vr_entry]
type: "Alpha"
[.vr_entry]
vr_type: 7
*[vr_entry]
type: "Alpha"
[.vr_entry]
vr_type: 7
[app]
vr_key: "Alpha,7"
type: "Alpha"
vr_type: 7
)"_el));
        WITH_CONTEXT(requireError("duplicate"));
        WITH_CONTEXT(requirePass(valid));
    }

    void testIndexesInsideEntriesKeepTheirLocalScope() {
        WITH_CONTEXT(requireRulesPass(R"(
[vr_literal_type]
type: "section_list"
*[vr_literal_type.vr_entry.vr_key]
name: "vr_vr_index"
key: "vr_entry.vr_entry.vr_name"
[vr_literal_type.vr_entry.vr_vr_entry]
type: "section_list"
[vr_literal_type.vr_entry.vr_vr_entry.vr_entry.vr_literal_vr_name]
type: "text"
[vr_literal_type.vr_entry.vr_literal_title]
type: "text"
key: "vr_vr_index"
)"_el));
        WITH_CONTEXT(requirePass(R"(
*[type]
title: "FIRST"
*[.vr_entry]
vr_name: "first"
*[type]
title: "second"
*[.vr_entry]
vr_name: "second"
)"_el));
        WITH_CONTEXT(requireFail(R"(
*[type]
title: "second"
*[.vr_entry]
vr_name: "first"
*[type]
title: "second"
*[.vr_entry]
vr_name: "second"
)"_el));
        WITH_CONTEXT(requireError("existing key"));
    }

    auto keyRules(const el::text::String &key, const el::text::String &reference = "index"_el) -> el::text::String {
        return el::text::StringFormat{R"(
*[vr_key]
name: "index"
key: "{}"
[vr_literal_type]
type: "section_list"
[vr_literal_type.vr_entry.vr_vr_type]
type: "text"
[vr_literal_type.vr_entry.vr_literal_vr_entry]
type: "section_list"
[vr_literal_type.vr_entry.vr_literal_vr_entry.vr_entry.vr_literal_type]
type: "text"
[app.selected]
type: "text"
key: "{}"
)"_el}
            .build(key, reference);
    }

    void testInvalidKeyPathsAndReferences() {
        const std::array<el::text::String, 8> paths{
            "vr_literal_type.vr_entry.vr_type"_el,
            "type.vr_entry.vr_vr_type"_el,
            "type.vr_type"_el,
            "type.vr_entry"_el,
            "type.vr_entry.unknown"_el,
            "type.vr_entry.vr_entry.vr_entry.type"_el,
            "type[0].vr_type"_el,
            "type.vr_entry.vr_type.extra"_el,
        };
        for (const auto &path : paths) {
            WITH_CONTEXT(requireRulesFail(keyRules(path)));
        }
        WITH_CONTEXT(requireRulesFail(keyRules("type.vr_entry.vr_type"_el, "index[1]"_el)));
        WITH_CONTEXT(requireError("out of bounds"));
        WITH_CONTEXT(requireRulesFail(keyRules("type.vr_entry.vr_type"_el, "vr_literal_index"_el)));
        WITH_CONTEXT(requireError("not found"));
        WITH_CONTEXT(requireRulesFail(keyRules("type.vr_entry.vr_type"_el, "index.extra"_el)));
        WITH_CONTEXT(requireError("index"));
    }

    void testDifferentListsAndInvalidKeyTypes() {
        WITH_CONTEXT(requireRulesFail(R"(
*[vr_key]
key: "type.vr_entry.vr_type", "vr_type.vr_entry.type"
[vr_literal_type]
type: "section_list"
[vr_literal_type.vr_entry.vr_vr_type]
type: "text"
[vr_vr_type]
type: "section_list"
[vr_vr_type.vr_entry.vr_literal_type]
type: "text"
)"_el));
        WITH_CONTEXT(requireError("same section list"));
        WITH_CONTEXT(requireRulesFail(R"(
*[vr_key]
key: "type.vr_entry.vr_type"
[vr_literal_type]
type: "section_list"
[vr_literal_type.vr_entry.vr_vr_type]
type: "boolean"
)"_el));
        WITH_CONTEXT(requireError("text or integer"));
        WITH_CONTEXT(requireRulesFail(R"(
*[vr_key]
key: "type.vr_entry"
[vr_literal_type]
type: "value_list"
[vr_literal_type.vr_entry]
type: "text"
)"_el));
        WITH_CONTEXT(requireError("section list"));
    }

    void testDependenciesAndOptionalAncestors() {
        WITH_CONTEXT(requireRulesPass(R"(
[app.vr_literal_type]
type: "section"
is_optional: Yes
[app.vr_literal_type.vr_vr_type]
type: "text"
[app.vr_vr_entry]
type: "text"
is_optional: Yes
[app.vr_literal_vr_literal_type]
type: "text"
default: "ready"
*[app.vr_dependency]
mode: "if"
source: "type.vr_type"
target: "vr_entry", "vr_literal_type"
)"_el));
        WITH_CONTEXT(requirePass("[app]\nvr_entry: \"present\"\n[app.type]\nvr_type: \"source\"\n"_el));
        REQUIRE_EQUAL(document->getText("app.vr_literal_type"_el), "ready"_el);
        WITH_CONTEXT(requireFail("[app.type]\nvr_type: \"source\"\n"_el));
        WITH_CONTEXT(requirePass("[app]\n"_el));
        WITH_CONTEXT(requireRulesFail(R"(
[app]
type: "section"
is_optional: Yes
[app.vr_literal_type]
type: "text"
[app.vr_vr_type]
type: "text"
is_optional: Yes
*[app.vr_dependency]
mode: "if"
source: "type"
target: "vr_type"
)"_el));
        WITH_CONTEXT(requireError("neither optional nor has a default"));
    }

    void testDependenciesRejectDefinitionSpellingsAndListCrossings() {
        const std::array<el::text::String, 3> sources{
            "vr_literal_type"_el, "vr_vr_type"_el, "vr_entry.vr_entry.type"_el};
        for (const auto &source : sources) {
            WITH_CONTEXT(requireRulesFail(
                el::text::StringFormat{R"(
[vr_literal_type]
type: "text"
is_optional: Yes
[vr_vr_type]
type: "text"
is_optional: Yes
[vr_literal_vr_entry]
type: "section_list"
is_optional: Yes
[vr_literal_vr_entry.vr_entry.vr_literal_type]
type: "text"
is_optional: Yes
*[vr_dependency]
mode: "if"
source: "{}"
target: "vr_type"
)"_el}
                    .build(source)));
        }
    }

    void testBuilderKeyAndDependencyNamesStayLiteral() {
        using namespace vr::builder;
        auto builder = vr::RulesBuilder{};
        builder.configureRoot(
            KeyIndex("vr_literal_index"_el, "app.vr_vr_list.vr_entry.vr_literal_type"_el),
            Dependency(vr::DependencyMode::If, {"app.vr_vr_source"_el}, {"app.vr_literal_target"_el}));
        builder.addRule("app"_el, vr::RuleType::Section);
        builder.addRule("app.vr_vr_list"_el, vr::RuleType::SectionList);
        builder.addRule("app.vr_vr_list.vr_entry"_el, vr::RuleType::Section);
        builder.addRule("app.vr_vr_list.vr_entry.vr_literal_type"_el, vr::RuleType::Text);
        builder.addRule("app.vr_vr_source"_el, vr::RuleType::Text, IsOptional(), ConfKey("vr_literal_index"_el));
        builder.addRule("app.vr_literal_target"_el, vr::RuleType::Text, IsOptional());
        rules = builder.takeRules();
        WITH_CONTEXT(requirePass(
            "[app]\nvr_vr_source: \"a\"\nvr_literal_target: \"b\"\n*[app.vr_vr_list]\nvr_literal_type: \"a\"\n"_el));
        WITH_CONTEXT(requireFail("[app]\nvr_vr_source: \"a\"\n*[app.vr_vr_list]\nvr_literal_type: \"a\"\n"_el));
        WITH_CONTEXT(requireFail(
            "[app]\nvr_vr_source: \"missing\"\nvr_literal_target: \"b\"\n*[app.vr_vr_list]\nvr_literal_type: \"a\"\n"_el));
        WITH_CONTEXT(requireError("existing key"));
    }

    void testTemplateKeysAndAlternativeFields() {
        WITH_CONTEXT(requireRulesPass(R"(
[vr_template.vr_vr_settings]
type: "section"
*[vr_template.vr_vr_settings.vr_key]
name: "vr_vr_index"
key: "vr_type.vr_entry.type"
[vr_template.vr_vr_settings.vr_vr_type]
type: "section_list"
*[vr_template.vr_vr_settings.vr_vr_type.vr_entry.vr_literal_type]
type: "boolean"
*[vr_template.vr_vr_settings.vr_vr_type.vr_entry.vr_literal_type]
type: "text"
*[vr_template.vr_vr_settings.vr_vr_type.vr_entry.vr_literal_type]
type: "integer"
[vr_template.vr_vr_settings.vr_literal_title]
type: "text"
key: "vr_vr_index"
[vr_template.vr_vr_settings.vr_literal_vr_dependency]
type: "text"
is_optional: Yes
[vr_template.vr_vr_settings.vr_vr_template]
type: "text"
is_optional: Yes
*[vr_template.vr_vr_settings.vr_dependency]
mode: "if"
source: "vr_dependency"
target: "vr_template"
[app.vr_literal_type]
use_template: "vr_vr_settings"
[app.vr_vr_type]
use_template: "vr_vr_settings"
)"_el));
        WITH_CONTEXT(requirePass(R"(
[app.type]
title: "first"
*[.vr_type]
type: "first"
[app.vr_type]
title: "7"
*[.vr_type]
type: 7
)"_el));
        WITH_CONTEXT(requireFail(R"(
[app.type]
title: "7"
*[.vr_type]
type: "first"
[app.vr_type]
title: "7"
*[.vr_type]
type: 7
)"_el));
        WITH_CONTEXT(requireError("existing key"));
    }

    void testControlNamesAsAlternativeTemplateIdentifiers() {
        for (const auto &name : {"vr_key"_el, "vr_dependency"_el}) {
            WITH_CONTEXT(requireRulesPass(
                el::text::StringFormat{R"(
*[vr_template.{0}]
type: "text"
*[vr_template.{0}]
type: "integer"
[app.vr_literal_type]
use_template: "{0}"
)"_el}
                    .build(name)));
            WITH_CONTEXT(requirePass("[app]\ntype: 7\n"_el));
            WITH_CONTEXT(requirePass("[app]\ntype: \"text\"\n"_el));
            WITH_CONTEXT(requireFail("[app]\ntype: Yes\n"_el));
        }
    }

    void testDefinitionAndDocumentDiagnosticPaths() {
        const auto duplicate = Parser{}.parseTextOrThrow(R"([app.vr_vr_type]
type: "text"
[app.vr_literal_vr_type]
type: "text"
)"_el);
        try {
            const auto invalidRules = vr::Rules::createFromDocument(duplicate);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE(error.category() == ConfErrorCategory::Validation);
            REQUIRE(error.context().namePath().has_value());
            REQUIRE_EQUAL(error.context().namePath()->toText(), "app.vr_literal_vr_type"_el);
            REQUIRE_EQUAL(
                error.location(), duplicate->valueOrThrow("app.vr_literal_vr_type"_el)->location().codeLocation());
        }
        WITH_CONTEXT(requireRulesPass("[app.vr_literal_type]\ntype: \"integer\"\n"_el));
        const auto invalid = Parser{}.parseTextOrThrow("[app]\ntype: \"wrong\"\n"_el);
        try {
            rules->validate(invalid, 0);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE(error.category() == ConfErrorCategory::Validation);
            REQUIRE(error.context().namePath().has_value());
            REQUIRE_EQUAL(error.context().namePath()->toText(), "app.type"_el);
            REQUIRE_EQUAL(error.location(), invalid->valueOrThrow("app.type"_el)->location().codeLocation());
        }
    }

    void testDefinitionErrorsRetainSourceLocations() {
        const auto source = Parser{}.parseTextOrThrow(R"([app]
type: "section"
[app.vr_literal_type]
type: "text"
*[app.vr_key]
key: "type.vr_entry.missing"
)"_el);
        try {
            const auto invalidRules = vr::Rules::createFromDocument(source);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE(error.category() == ConfErrorCategory::Validation);
            REQUIRE_EQUAL(error.location(), source->valueOrThrow("app.vr_key[0]"_el)->location().codeLocation());
            REQUIRE(error.description().contains("type.vr_entry.missing"_el));
        }
    }
};
