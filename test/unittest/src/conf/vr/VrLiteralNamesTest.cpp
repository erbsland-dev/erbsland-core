// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VrBase.hpp"

#include <erbsland/conf/vr/RulesBuilder.hpp>

TESTED_TARGETS(Rules RulesBuilder)
TAGS(ValidationRules)
class VrLiteralNamesTest final : public UNITTEST_SUBCLASS(VrBase) {
private:
    static constexpr auto cAppType = "app.type"_el;

public:
    void testAttributeNamesRequiredAndDefaults() {
        WITH_CONTEXT(requireRulesPass(R"(
[server]
type: "section"
title: "Server settings"
[server.vr_literal_type]
type: "text"
[server.vr_literal_title]
type: "text"
default: "default title"
[server.vr_vr_type]
type: "integer"
[server.vr_vr_name]
type: "text"
default: "literal name"
)"_el));
        WITH_CONTEXT(requirePass("[server]\ntype: \"http\"\nvr_type: 42\n"_el));
        REQUIRE_EQUAL(document->getText("server.title"_el), "default title"_el);
        REQUIRE_EQUAL(document->getText("server.vr_name"_el), "literal name"_el);
        WITH_CONTEXT(requireFail("[server]\nvr_type: 42\n"_el));
        WITH_CONTEXT(requireError("'type'"));
        WITH_CONTEXT(requireFail("[server]\ntype: \"http\"\n"_el));
        WITH_CONTEXT(requireError("'vr_type'"));
    }
    void testTemplateInsertionAndAlternatives() {
        WITH_CONTEXT(requireRulesPass(R"(
[vr_template.settings]
type: "section"
[vr_template.settings.vr_literal_type]
type: "text"
[vr_template.settings.vr_vr_type]
type: "integer"
[app.vr_literal_type]
use_template: "settings"
*[app.vr_literal_title]
type: "text"
*[app.vr_literal_title]
type: "integer"
[app.vr_literal_vr_literal_type]
type: "text"
)"_el));
        WITH_CONTEXT(
            requirePass("[app]\ntitle: 1\nvr_literal_type: \"raw\"\n[app.type]\ntype: \"nested\"\nvr_type: 2\n"_el));
        const auto value = document->valueOrThrow("app.type.vr_type"_el);
        REQUIRE_EQUAL(value->validationRule()->namePath().toText(), "app.type.vr_type"_el);
        WITH_CONTEXT(
            requireFail("[app]\ntitle: Yes\nvr_literal_type: \"raw\"\n[app.type]\ntype: \"nested\"\nvr_type: 2\n"_el));
    }
    void testDuplicateTargetsAndInvalidSuffixes() {
        WITH_CONTEXT(requireRulesFail("[app.value]\ntype: \"text\"\n[app.vr_literal_value]\ntype: \"text\"\n"_el));
        WITH_CONTEXT(requireError("same target name"));
        WITH_CONTEXT(
            requireRulesFail("[app.vr_vr_type]\ntype: \"text\"\n[app.vr_literal_vr_type]\ntype: \"text\"\n"_el));
        WITH_CONTEXT(requireError("same target name"));
        WITH_CONTEXT(requireRulesFail("[app.vr_literal_1value]\ntype: \"text\"\n"_el));
        WITH_CONTEXT(requireError("suffix must be a regular name"));
    }
    void testDependenciesUseActualNames() {
        WITH_CONTEXT(requireRulesPass(R"(
[app]
type: "section"
[app.vr_literal_type]
type: "text"
is_optional: Yes
[app.vr_vr_type]
type: "text"
is_optional: Yes
[app.vr_literal_vr_entry]
type: "text"
is_optional: Yes
*[app.vr_dependency]
mode: "if"
source: "type", "vr_type"
target: "vr_entry"
)"_el));
        WITH_CONTEXT(requirePass("[app]\ntype: \"x\"\nvr_type: \"y\"\nvr_entry: \"z\"\n"_el));
        WITH_CONTEXT(requireFail("[app]\ntype: \"x\"\n"_el));
        WITH_CONTEXT(requireFail("[app]\nvr_type: \"y\"\n"_el));
        WITH_CONTEXT(requirePass("[app]\n"_el));
    }
    void testKeysUseActualNamesAndContextualMarkers() {
        WITH_CONTEXT(requireRulesPass(R"(
*[vr_key]
name: "kinds"
key: "vr_entry.vr_entry.type", "vr_entry.vr_entry.vr_type", "vr_entry.vr_entry.vr_entry"
[vr_literal_vr_entry]
type: "section_list"
[vr_literal_vr_entry.vr_entry.vr_literal_type]
type: "text"
[vr_literal_vr_entry.vr_entry.vr_vr_type]
type: "text"
[vr_literal_vr_entry.vr_entry.vr_vr_entry]
type: "text"
[app.selected]
type: "text"
key: "kinds[0]"
)"_el));
        WITH_CONTEXT(
            requirePass("*[vr_entry]\ntype: \"x\"\nvr_type: \"y\"\nvr_entry: \"z\"\n[app]\nselected: \"x\"\n"_el));
        WITH_CONTEXT(requireFail(
            "*[vr_entry]\ntype: \"x\"\nvr_type: \"y\"\nvr_entry: \"z\"\n[app]\nselected: \"missing\"\n"_el));
        WITH_CONTEXT(requireError("refer to an existing key"));
        WITH_CONTEXT(requireFail(
            "*[vr_entry]\ntype: \"x\"\nvr_type: \"y\"\nvr_entry: \"z\"\n*[vr_entry]\ntype: \"x\"\nvr_type: \"y\"\nvr_entry: \"z\"\n[app]\nselected: \"x\"\n"_el));
        WITH_CONTEXT(requireError("duplicate"));
    }
    void testProgrammaticNamesAreLiteral() {
        auto builder = vr::RulesBuilder{};
        builder.addRule("app"_el, vr::RuleType::Section);
        builder.addRule("app.type"_el, vr::RuleType::Text);
        builder.addRule("app.vr_literal_type"_el, vr::RuleType::Text);
        builder.addRule("app.vr_vr_type"_el, vr::RuleType::Text);
        rules = builder.takeRules();
        WITH_CONTEXT(requirePass("[app]\ntype: \"a\"\nvr_literal_type: \"b\"\nvr_vr_type: \"c\"\n"_el));
        WITH_CONTEXT(requireFail("[app]\ntype: \"a\"\nvr_literal_type: \"b\"\n"_el));
    }
    void testEscapesAreDecodedOnceAtEveryDepth() {
        WITH_CONTEXT(requireRulesPass(R"(
[VR Literal Server.VR VR Type.VR Literal Title]
type: "text"
[vr_literal_server.vr_vr_type.vr_vr_vr_type]
type: "text"
[vr_literal_server.vr_vr_type.vr_literal_vr_literal_type]
type: "text"
[vr_literal_server.vr_vr_type.vr_literal_vr_vr_name]
type: "text"
)"_el));
        WITH_CONTEXT(requirePass(R"(
[Server.VR Type]
Title: "a"
vr_vr_type: "b"
vr_literal_type: "c"
vr_vr_name: "d"
)"_el));
        WITH_CONTEXT(requireFail(R"(
[server.vr_type]
title: "a"
vr_type: "b"
vr_literal_type: "c"
vr_vr_name: "d"
)"_el));
        WITH_CONTEXT(requireRulesFail(R"(
[app.vr_vr_literal_type]
type: "text"
[app.vr_literal_vr_literal_type]
type: "text"
)"_el));
        WITH_CONTEXT(requireError("same target name"));
        WITH_CONTEXT(requireRulesFail(R"(
[app.Type]
type: "text"
[app.VR Literal Type]
type: "text"
)"_el));
        WITH_CONTEXT(requireError("same target name"));
    }

    void testLiteralControlNamesCoexistWithControls() {
        WITH_CONTEXT(requireRulesPass(R"(
[app]
type: "section"
[app.vr_name]
starts: "app"
[app.vr_any]
type: "boolean"
[app.vr_literal_vr_any]
type: "text"
[app.vr_vr_entry]
type: "integer"
[app.vr_literal_vr_name]
type: "integer"
default: 7
[app.vr_vr_key]
type: "text"
[app.vr_literal_vr_dependency]
type: "text"
is_optional: Yes
[app.vr_vr_template]
type: "text"
is_optional: Yes
*[app.vr_dependency]
mode: "if"
source: "vr_dependency"
target: "vr_template"
)"_el));
        WITH_CONTEXT(requirePass(R"(
[app]
vr_any: "literal"
vr_entry: 4
vr_key: "index name"
vr_dependency: "present"
vr_template: "present"
another: Yes
)"_el));
        REQUIRE_EQUAL(document->getInteger("app.vr_name"_el), 7);
        WITH_CONTEXT(requireFail(R"(
[app]
vr_any: Yes
vr_entry: 4
vr_key: "index name"
)"_el));
        WITH_CONTEXT(requireFail(R"(
[app]
vr_entry: 4
vr_key: "index name"
)"_el));
        WITH_CONTEXT(requireError("vr_any"));
    }

    void testEscapedAlternativesDefaultsAndVersions() {
        WITH_CONTEXT(requireRulesPass(R"(
*[app.vr_literal_type]
type: "text"
default: "fallback"
*[app.vr_literal_type]
type: "integer"
[app.vr_vr_type]
type: "text"
version: 2
[app.vr_literal_title]
type: "text"
is_optional: Yes
)"_el));
        WITH_CONTEXT(requirePass("[app]\n"_el, 1));
        REQUIRE_EQUAL(document->getText(cAppType), "fallback"_el);
        WITH_CONTEXT(requirePass("[app]\ntype: 7\nvr_type: \"enabled\"\n"_el, 2));
        WITH_CONTEXT(requireFail("[app]\ntype: 7\n"_el, 2));
        WITH_CONTEXT(requireError("vr_type"));
    }

    void testTemplateExpansionDuplicateValueNames() {
        WITH_CONTEXT(requireRulesFail(R"(
[vr_template.vr_literal_settings]
type: "section"
[vr_template.vr_literal_settings.vr_vr_type]
type: "text"
[app.vr_literal_type]
use_template: "vr_literal_settings"
[app.vr_literal_type.vr_literal_vr_type]
type: "text"
)"_el));
        WITH_CONTEXT(requireError("same target name"));
    }
};
