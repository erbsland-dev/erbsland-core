// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormatForConf.hpp>
#include <erbsland/conf/vr/Rules.hpp>

#include <array>
#include <string_view>

using namespace el::conf;
using namespace el::text::literals;

// Testing all requirements from chapter "Validation Rules"->"Node-Rules Definition".
// As this is an overview chapter, we only test the most basic requirements.
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrNodeRulesDefinitionTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void requireSecretValueRoundTrip(const std::string_view typeLine, const std::string_view valueLine) {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.secret]",
            typeLine,
            "is_secret: yes",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            valueLine,
        }));
        auto value = document->valueOrThrow(el::text::String{"app.secret"});
        REQUIRE(value->validationRule() != nullptr);
        REQUIRE(value->validationRule()->isSecret());
        REQUIRE(value->isSecret());
    }

    void testTypeRequirement() {
        // Type Requirement: Each node-rules definition must have exactly one effective type.
        // The type is provided either by:
        // - a local type field, or
        // - a use_template reference that defines a type.

        // test first a valid definition.
        WITH_CONTEXT(requireRulesPassLines({
            "[client.port]",
            "type: \"integer\"",
        }));
        // without type, it must fail.
        WITH_CONTEXT(requireRulesFailLines({
            "[client.port]",
        }));
        WITH_CONTEXT(requireError("The section must have either a 'type' or a 'use_template' value"));

        // The same is true for templates.
        WITH_CONTEXT(requireRulesPassLines({
            "[vr_template.a]",
            "type: \"integer\"",
            "[client.port]",
            "use_template: \"a\"",
        }));

        // If the template has no type, it must fail.
        WITH_CONTEXT(requireRulesFailLines({
            "[vr_template.a]",
            "[client.port]",
            "use_template: \"a\"",
        }));
        WITH_CONTEXT(requireError("The section must have either a 'type' or a 'use_template' value"));

        // will be tested more thoughtfully in the `type` tests.
    }

    void testDefaults() {
        // Defaults: Node-rules definitions for a scalar value or a value list may define a default value
        // using the `default` field.

        // default for scalars
        WITH_CONTEXT(requireRulesPassLines({
            "[client.port]",
            "type: \"integer\"",
            "default: 9000",
        }));
        WITH_CONTEXT(requirePassLines({
            "[client]",
            "port: 1",
        }));
        WITH_CONTEXT(requirePassLines({
            "[client]",
        }));
        REQUIRE_EQUAL(document->getOrThrow<Integer>(el::text::String{"client.port"}), 9000);

        // `section` is not automatically made optional by giving `port` a default value.
        WITH_CONTEXT(requireFailLines({}));
        WITH_CONTEXT(requireError("expected a section with the name 'client'"));
        // The `default` type is validated. Therefore `default` must match `type`.
        WITH_CONTEXT(requireRulesFailLines({
            "[main.a]",
            "type: \"integer\"",
            "default: \"text\"",
        }));
        WITH_CONTEXT(requireError("The 'default' value must be an integer value"));

        // default for value lists and value matrices
        WITH_CONTEXT(requireRulesPassLines({
            "[main.a]",
            "type: \"value_list\"",
            "default: 10, 20, 30", // accepting a value list as default.
            "[.vr_entry]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireRulesPassLines({
            "[main.a]",
            "type: \"value_list\"",
            "default: 10", // 10, even being a scalar, is also a value list with one element.
            "[.vr_entry]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireRulesPassLines({
            "[main.a]",
            "type: \"value_matrix\"",
            "default:", // accepting a value matrix as default.
            "    * 1, 2, 3",
            "    * 4, 5, 6",
            "    * 7, 8, 9",
            "[.vr_entry]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireRulesPassLines({
            "[main.a]",
            "type: \"value_list\"",
            "default: 1", // a single scalar is also a value matrix with one row and one column.
            "[.vr_entry]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireRulesPassLines({
            "[main.a]",
            "type: \"value_list\"",
            "default: 1, 2, 3", // a value list is also a value matrix with one column.
            "[.vr_entry]",
            "type: \"integer\"",
        }));

        // `Value` allows any scalar value, so a scalar `default` value should be allowed.
        WITH_CONTEXT(requireRulesPassLines({
            "[main.a]",
            "type: \"value\"",
            "default: 1",
        }));

        // defaults for all other types must not be allowed.
        // default for value lists and value matrices
        WITH_CONTEXT(requireRulesFailLines({
            "[main.a]",
            "type: \"section\"",
            "default: 1", // whatever default this would take.
        }));
        // make sure this is failing because of the rule type and not because of the type mismatch.
        WITH_CONTEXT(requireError("A default value cannot be used for 'Section' node rules"));
        // test other types as well.
        WITH_CONTEXT(requireRulesFailLines({
            "[main.a]",
            "type: \"section_list\"",
            "default: 1",
            "[.vr_entry]",
            "type: \"section\"",
        }));
        WITH_CONTEXT(requireError("A default value cannot be used for 'SectionList' node rules"));
        WITH_CONTEXT(requireRulesFailLines({
            "[main.a]",
            "type: \"section_with_texts\"",
            "default: 1",
        }));
        WITH_CONTEXT(requireError("A default value cannot be used for 'SectionWithTexts' node rules"));
        WITH_CONTEXT(requireRulesFailLines({
            "[main.a]",
            "type: \"not_validated\"",
            "default: 1",
        }));
        WITH_CONTEXT(requireError("A default value cannot be used for 'NotValidated' node rules"));
    }

    void testOptionality() {
        // Optionality: A node-rules definition may explicitly mark a node as optional using `is_optional: yes`.
        WITH_CONTEXT(requireRulesPassLines({
            "[client]",
            "type: \"section\"",
            "is_optional: yes",
        }));
        WITH_CONTEXT(requirePassLines({}));
        WITH_CONTEXT(requirePassLines({
            "[client]",
        }));
    }

    void testDocumentationFields() {
        // Documentation Fields: A node-rules definition may provide documentation metadata using
        // the `title` and `description` fields.
        WITH_CONTEXT(requireRulesPassLines({
            "[client.port]",
            "type: \"integer\"",
            "title: \"Port on the server to connect to\"",
            "description: \"The numeric port where the client connects to the server.\"",
            "default: 9000",
        }));
        WITH_CONTEXT(requirePassLines({
            "[client]",
            "port: 1",
        }));
        // now test if the documentation is accessible via public API.
        auto portValue = document->valueOrThrow(el::text::String{"client.port"_el});
        auto portValueRule = portValue->validationRule();
        REQUIRE(portValueRule != nullptr);
        REQUIRE_EQUAL(portValueRule->title(), el::text::String{"Port on the server to connect to"});
        REQUIRE_EQUAL(
            portValueRule->description(),
            el::text::String{"The numeric port where the client connects to the server."});
    }

    void testConstraints() {
        // Constraints: A node-rules definition can define zero or more constraints that further restrict
        // valid values or structures.
        WITH_CONTEXT(requireRulesPassLines({
            "[client.port]",
            "type: \"integer\"",
            "minimum: 1",
            "maximum: 65534",
        }));
        WITH_CONTEXT(requirePassLines({
            "[client]",
            "port: 1",
        }));
        WITH_CONTEXT(requireFailLines({
            "[client]",
            "port: 0",
        }));
        WITH_CONTEXT(requireError("The value must be at least 1"));
        WITH_CONTEXT(requireFailLines({
            "[client]",
            "port: 10'000'000",
        }));
        WITH_CONTEXT(requireError("The value must be at most 65534"));
    }

    void testCustomError() {
        // Custom ConfError: A node-rules definition may define a custom error message for the entire
        // node using the `error` field.
        WITH_CONTEXT(requireRulesPassLines({
            "[client.port]",
            "type: \"integer\"",
            "minimum: 1",
            "maximum: 65534",
            "error: \"Please specify a valid port from 1–65534\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[client]",
            "port: 0",
        }));
        WITH_CONTEXT(requireError("Please specify a valid port from 1–65534"));
        // also fails, but `type` is no constraint, so `error` has no effect.
        WITH_CONTEXT(requireFailLines({
            "[client]",
            "port: \"text\"",
        }));
        WITH_CONTEXT(requireError("Expected an integer value but got a text value"));
    }

    void testSectionListEntries() {
        // List Entries: Node-rules definitions for value lists or section lists must describe their elements
        // using a vr_entry subsection or section list.
        WITH_CONTEXT(requireRulesPassLines({
            "[client]",
            "type: \"section_list\"",
            "[client.vr_entry.name]",
            "type: \"text\"",
            "[client.vr_entry.port]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[client]*",
            "name: \"example\"",
            "port: 1234",
            "*[client]*",
            "name: \"example2\"",
            "port: 5678",
            "*[client]*",
            "name: \"example3\"",
            "port: 9012",
        }));
        // Must fail if `vr_entry` is missing.
        WITH_CONTEXT(requireRulesFailLines({
            "[client]",
            "type: \"section_list\"",
        }));
        WITH_CONTEXT(requireError("A section list rule must have a 'vr_entry' node-rules definition"));
        // Must fail if `vr_entry` is no section or section with texts.
        WITH_CONTEXT(requireRulesFailLines({
            "[client]",
            "type: \"section_list\"",
            "[client.vr_entry]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireError(
            "The 'vr_entry' node-rules definition for a section list must be of type 'section' or "
            "'section_with_texts'"));
        // Must fail a section list contains unrelated subsections.
        WITH_CONTEXT(requireRulesFailLines({
            "[client]",
            "type: \"section_list\"",
            "[client.vr_entry]",
            "type: \"section\"",
            "[client.count]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireError("only 'vr_entry' is permitted"));
    }

    void testValueListEntries() {
        // value lists without `vr_entry` are not valid.
        WITH_CONTEXT(requireRulesFailLines({
            "[client]",
            "type: \"value_list\"",
        }));
        WITH_CONTEXT(requireError("A value list or matrix rule must have a 'vr_entry' node-rules definition"));
        // value lists with `vr_entry` must be scalar types.
        WITH_CONTEXT(requireRulesPassLines({
            "[client]",
            "type: \"value_list\"",
            "[client.vr_entry]",
            "type: \"integer\"",
        }));
        // must fail for other types.
        WITH_CONTEXT(requireRulesFailLines({
            "[client]",
            "type: \"value_list\"",
            "[client.vr_entry]",
            "type: \"section\"",
        }));
        WITH_CONTEXT(requireError("Unexpected 'vr_entry' node-rules definition type for a value list"));
    }

    // skip dependencies, too complex for a simple test

    void testVariableNames() {
        // Variable Names: Node-rules definitions that end in vr_any may restrict the allowed
        // variable names using a `vr_name` subsection.
        WITH_CONTEXT(requireRulesPassLines({
            "[main.vr_any]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[main]",
        }));
        WITH_CONTEXT(requirePassLines({
            "[main]",
            "a: \"example\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[main]",
            "a: \"example\"",
            "b: \"example\"",
            "c: \"example\"",
            "d: \"example\"",
            "e: \"example\"",
            "f: \"example\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[main]",
            "a: \"example\"",
            "b: \"example\"",
            "c: 123",
        }));
        WITH_CONTEXT(requireError("Expected a text value but got an integer value"));
    }

    void testVariableNamesWithNameConstraints() {
        WITH_CONTEXT(requireRulesPassLines({
            "[main.vr_any]",
            "type: \"text\"",
            "[main.vr_any.vr_name]",
            "maximum: 1",
        }));
        WITH_CONTEXT(requirePassLines({
            "[main]",
        }));
        WITH_CONTEXT(requirePassLines({
            "[main]",
            "a: \"example\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[main]",
            "a: \"example\"",
            "b: \"example\"",
            "example: \"example\"",
        }));
        WITH_CONTEXT(requireError("Value name validation failed:"));
    }

    void testCaseSensitivity() {
        // Case Sensitivity: A node-rules definition may enable case-sensitive evaluation for text-based
        // constraints using the case_sensitive field.
        WITH_CONTEXT(
            requireRulesPassLines({"[app.message]", "type: \"text\"", "case_sensitive: yes", "starts: \"message:\""}));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "message: \"message: Hello World\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "message: \"MESSAGE: Hello World\"",
        }));

        WITH_CONTEXT(
            requireRulesPassLines({"[app.message]", "type: \"text\"", "case_sensitive: no", "starts: \"message:\""}));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "message: \"message: Hello World\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "message: \"MESSAGE: Hello World\"",
        }));
    }

    void testSecrets() {
        // Secrets: A node-rules definition may mark a node as secret using the is_secret field.
        // Security spec: it applies to scalar values.
        const std::array<std::pair<std::string_view, std::string_view>, 4> secretScalarCases = {{
            {"type: \"text\"", "secret: \"secret_password\""},
            {"type: \"integer\"", "secret: 1234"},
            {"type: \"boolean\"", "secret: yes"},
            {"type: \"value\"", "secret: \"secret_password\""},
        }};
        for (const auto &[typeLine, valueLine] : secretScalarCases) {
            WITH_CONTEXT(requireSecretValueRoundTrip(typeLine, valueLine));
        }

        WITH_CONTEXT(requireRulesPassLines({
            "[app.secret]",
            "type: \"text\"",
            "is_secret: no",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "secret: \"public\"",
        }));
        REQUIRE_FALSE(document->valueOrThrow(el::text::String{"app.secret"})->isSecret());

        WITH_CONTEXT(requireRulesFailLines({
            "[app.secret]",
            "type: \"text\"",
            "is_secret: \"yes\"",
        }));
        WITH_CONTEXT(requireError("The 'is_secret' value must be boolean"));

        // Security spec: is_secret may only be set for scalar values.
        WITH_CONTEXT(requireRulesFailLines({
            "[app.secret]",
            "type: \"section\"",
            "is_secret: yes",
        }));
    }
};
