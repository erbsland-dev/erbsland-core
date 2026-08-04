// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/impl/lexer/Name.hpp>
#include <erbsland/conf/impl/value/Section.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;

// Testing all requirements from chapter "Validation Rules"->"Templates".
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrTemplatesTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
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
