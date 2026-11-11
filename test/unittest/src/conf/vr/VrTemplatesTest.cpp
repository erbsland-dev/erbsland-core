// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/impl/lexer/Name.hpp>
#include <erbsland/conf/impl/value/Section.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/StringFormat.hpp>

using namespace el::conf;

// Testing all requirements from chapter "Validation Rules"->"Templates".
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrTemplatesTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testNameTemplatesAndOverrides() {
        WITH_CONTEXT(requireRulesPass(R"(
[vr_template.identifier]
type: "text"
starts: "u_"
minimum: 3
case_sensitive: Yes
[users]
type: "section_with_texts"
[users.vr_any]
type: "integer"
[users.vr_any.vr_name]
use_template: "identifier"
minimum: 4
[tags]
type: "section_with_texts"
[tags.vr_any]
type: "text"
[tags.vr_any.vr_name]
use_template: "identifier"
case_sensitive: No
)"_el));
        WITH_CONTEXT(requirePass("[users]\n\"u_name\": 1\n[tags]\n\"U_x\": \"arbitrary value\"\n"_el));
        WITH_CONTEXT(requireFail("[users]\n\"u_x\": 1\n[tags]\n\"U_x\": \"value\"\n"_el));
        WITH_CONTEXT(requireFail("[users]\n\"U_name\": 1\n[tags]\n\"U_x\": \"value\"\n"_el));
    }

    void testInvalidNameTemplates() {
        for (const auto type : {"integer"_el, "section"_el, "value"_el}) {
            WITH_CONTEXT(requireRulesFail(
                el::text::StringFormat{
                    "[vr_template.bad]\ntype: \"{}\"\n[app.vr_any]\ntype: \"text\"\n[app.vr_any.vr_name]\nuse_template: \"bad\"\n"_el}
                    .build(type)));
            WITH_CONTEXT(requireError("templates must have a 'type' value of 'text'"));
        }
        WITH_CONTEXT(requireRulesFail(R"(
*[vr_template.bad]
type: "text"
[app.vr_any]
type: "text"
[app.vr_any.vr_name]
use_template: "bad"
)"_el));
        WITH_CONTEXT(requireError("cannot be alternatives"));
        WITH_CONTEXT(requireRulesFail(R"(
[vr_template.good]
type: "text"
[app.vr_any]
type: "text"
[app.vr_any.vr_name]
type: "text"
use_template: "good"
)"_el));
        WITH_CONTEXT(requireError("cannot have both"));
        WITH_CONTEXT(requireRulesFail(R"(
[vr_template.bad]
type: "text"
[vr_template.bad.child]
type: "text"
[app.vr_any]
type: "text"
[app.vr_any.vr_name]
use_template: "bad"
)"_el));
        WITH_CONTEXT(requireError("cannot have subsections"));
        WITH_CONTEXT(
            requireRulesFail("[app.vr_any]\ntype: \"text\"\n[app.vr_any.vr_name]\nuse_template: \"missing\"\n"_el));
        WITH_CONTEXT(requireError("does not exist"));
    }

    void testTemplatesInRoot() {
        // Templates in Root: Templates must be defined as subsections or section lists under "vr_template"
        // at the document root.
        WITH_CONTEXT(requireRulesPassLines({
            "[vr_template.interface]",
            "type: \"section\"",
            "[.address]",
            "type: \"text\"",
            "[.port]",
            "type: \"integer\"",
            "default: 443",
            "[server.interface]",
            "use_template: \"interface\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server.interface]",
            "address: \"localhost\"",
        }));
        const auto serverPort = document->getOrThrow<Integer>(el::text::String{"server.interface.port"});
        REQUIRE_EQUAL(serverPort, 443);

        // "vr_template" must only appear at the document root.
        WITH_CONTEXT(requireRulesFailLines({
            "[server.vr_template]",
            "type: \"section\"",
        }));
        WITH_CONTEXT(requireError("Templates must be defined in the document root"));
    }

    void testTemplateIdentifier() {
        // Template Identifier: The name of a subsection or section list under "vr_template" is
        // the identifier used to reference the template via "use_template".
        WITH_CONTEXT(requireRulesPassLines({
            "[vr_template.port]",
            "type: \"integer\"",
            "[server.port]",
            "use_template: \"port\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "port: 1",
        }));

        WITH_CONTEXT(requireRulesFailLines({
            "[server.port]",
            "use_template: \"missing\"",
        }));
        WITH_CONTEXT(requireError("The template referenced by 'use_template' does not exist"));

        // Template identifiers must be valid regular names.
        WITH_CONTEXT(requireRulesFailLines({
            "[server.port]",
            "use_template: \"+++\"",
        }));
        WITH_CONTEXT(requireError("The name specified in 'use_template' is not a valid template name"));
    }

    void testTemplateSameStructureAsNodeRules() {
        // Same Structure as Node-Rules: Templates are defined like node-rules definitions, including constraints.
        WITH_CONTEXT(requireRulesPassLines({
            "[vr_template.port]",
            "type: \"integer\"",
            "minimum: 1",
            "maximum: 65534",
            "[server.port]",
            "use_template: \"port\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "port: 1",
        }));
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "port: 0",
        }));
        WITH_CONTEXT(requireError("The value must be at least 1"));
    }

    void testTemplateDirectTypeDefinitionOnly() {
        // Direct Type Definition Only: A template must define its own type and must not use "use_template".
        WITH_CONTEXT(requireRulesFailLines({
            "[vr_template.port]",
            "type: \"integer\"",
            "[vr_template.client_port]",
            "use_template: \"port\"",
            "[server.port]",
            "use_template: \"client_port\"",
        }));
        WITH_CONTEXT(requireError("You must not use 'use_template' in template definitions"));
    }

    void testTemplateCopySemantics() {
        // Copy Semantics: When a template is applied, its contents are copied into the usage location.
        WITH_CONTEXT(requireRulesPassLines({
            "[vr_template.interface]",
            "type: \"section\"",
            "[.address]",
            "type: \"text\"",
            "[.port]",
            "type: \"integer\"",
            "default: 443",
            "[server.interface]",
            "use_template: \"interface\"",
            "[client.interface]",
            "use_template: \"interface\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server.interface]",
            "address: \"localhost\"",
            "[client.interface]",
            "address: \"localhost\"",
        }));
        const auto serverPort = document->getOrThrow<Integer>(el::text::String{"server.interface.port"});
        const auto clientPort = document->getOrThrow<Integer>(el::text::String{"client.interface.port"});
        REQUIRE_EQUAL(serverPort, 443);
        REQUIRE_EQUAL(clientPort, 443);
        // to validate the copy, compare the validation-rule instances.
        auto serverInterface = document->valueOrThrow(el::text::String{"server.interface"});
        auto clientInterface = document->valueOrThrow(el::text::String{"client.interface"});
        REQUIRE_NOT_EQUAL(serverInterface->validationRule(), nullptr);
        REQUIRE_NOT_EQUAL(clientInterface->validationRule(), nullptr);
        REQUIRE_NOT_EQUAL(serverInterface->validationRule(), clientInterface->validationRule());
    }

    void testOverridesAllowed() {
        // Overrides Allowed: Constraints at the usage location overwrite constraints from the template.
        WITH_CONTEXT(requireRulesPassLines({
            "[vr_template.port]",
            "type: \"integer\"",
            "minimum: 1",
            "maximum: 100",
            "[server.port]",
            "use_template: \"port\"",
            "minimum: 10",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "port: 10",
        }));
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "port: 5",
        }));
        WITH_CONTEXT(requireError("The value must be at least 10"));
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "port: 200",
        }));
        WITH_CONTEXT(requireError("The value must be at most 100"));
    }

    void testTemplatesAsSectionLists() {
        // Templates can be defined as section lists and used as alternatives.
        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_template.service]*",
            "type: \"integer\"",
            "minimum: 1",
            "*[vr_template.service]*",
            "type: \"text\"",
            "minimum: 1",
            "[server.service]",
            "use_template: \"service\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "service: 10",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "service: \"http\"",
        }));
    }

    void testAlternativesCannotBeOverwritten() {
        // Alternatives Cannot Be Overwritten: Constraints defined as alternatives must not be overwritten
        // at the usage location.
        WITH_CONTEXT(requireRulesFailLines({
            "*[vr_template.service]*",
            "type: \"integer\"",
            "minimum: 1",
            "*[vr_template.service]*",
            "type: \"text\"",
            "minimum: 1",
            "[server.service]",
            "use_template: \"service\"",
            "minimum: 1024",
        }));
        WITH_CONTEXT(requireError("Templates that define alternatives cannot be customized at the usage location"));
    }

    void testOrderOfOverrides() {
        // Order of Overrides and Additions: Constraints from a template are merged with additional constraints
        // in a predictable and stable order:
        // - Constraint overrides replace the corresponding constraint at the same position where the original
        //   constraint appeared in the template.
        // - New constraints that do not exist in the template are appended to the end of the resulting
        //   constraint list.
        WITH_CONTEXT(requireRulesPassLines({
            "[vr_template.port]",
            "type: \"integer\"",
            "minimum: 1",
            "maximum: 65534",
            "[server.port]",
            "use_template: \"port\"",
            "minimum: 10",
            "not_equals: 80",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "port: 10",
        }));
        auto portRule = document->valueOrThrow(el::text::String{"server.port"})->validationRule();
        auto constraints = portRule->constraints();
        REQUIRE_EQUAL(constraints.size(), 3);
        REQUIRE_EQUAL(constraints[0]->name(), el::text::String{"minimum"});
        REQUIRE_EQUAL(constraints[1]->name(), el::text::String{"maximum"});
        REQUIRE_EQUAL(constraints[2]->name(), el::text::String{"not_equals"});
    }
};
