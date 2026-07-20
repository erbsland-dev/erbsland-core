// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;

// Testing all requirements from chapter "Validation Rules"->"Evaluation Order".
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrEvaluationOrderTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testConstraintOrder() {
        // Order of Constraints: Constraints within a validation stage must be evaluated in the following order:
        // 1. The type constraint is always evaluated first.
        // 2. All remaining constraints are evaluated in the order of definition.
        // 3. During the second traversal, key constraints are evaluated first.
        // 4. Rules declared in vr_dependency are evaluated last.
        WITH_CONTEXT(requireRulesPassLines(
            {"[server.port]",
                "type: \"integer\"",
                "minimum: 1000",
                "not equals: 900",
                "not multiple: 7",
                "maximum: 2000"}));

        // Make sure we have one passing example.
        WITH_CONTEXT(requirePassLines({"[server]", "port = 1500"}));

        // `type` must be tested first.
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "port = \"text\"",
        }));
        WITH_CONTEXT(requireError("Expected an integer value but got a text value"));

        // `minimum` must be tested as first constraint.
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "port = 500",
        }));
        WITH_CONTEXT(requireError("The value must be at least 1000"));

        // not equals must be tested *after* `minimum`, so minimum will raise its error first.
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "port = 900",
        }));
        WITH_CONTEXT(requireError("The value must be at least 1000"));

        // `not multiple: 7` must be tested *before* the `maximum`.
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "port = 7000",
        }));
        WITH_CONTEXT(requireError("The value must not be a multiple of 7"));

        // test maximum as well.
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "port = 3000",
        }));
        WITH_CONTEXT(requireError("The value must be at most 2000"));
    }

    void testDefinitionOrder() {
        // Definition Order of Child Nodes: Child nodes must be processed in the order of their definition within
        // the configuration document.
        // ...and...
        // Bottom-Up Structure Traversal: The document structure must be traversed bottom up, completing validation
        // of each branch before moving on to unrelated branches.
        // This ensures that subsections—which may appear later in the document—are fully validated before sibling
        // sections that are defined earlier.
        WITH_CONTEXT(requireRulesPassLines({
            "[a.a]",
            "type: \"integer\"",
            "[a.b]",
            "type: \"integer\"",
            "[a.c.a]",
            "type: \"integer\"",
            "[b.a]",
            "type: \"integer\"",
            "[b.b]",
            "type: \"integer\"",
        }));

        // Test one passing example.
        WITH_CONTEXT(requirePassLines({
            "[a]",
            "a = 1",
            "b = 2",
            "[a.c]",
            "a = 3",
            "[b]",
            "a = 4",
            "b = 5",
        }));

        // test in which order the values of section [a] get tested:
        WITH_CONTEXT(requireFailLines({
            "[a]",
            "b = \"text\"",
            "a = \"text\"",
            "[a.c]",
            "a = 3",
            "[b]",
            "b = \"text\"",
            "a = \"text\"",
        }));
        // expect that "a.b" is failing first.
        WITH_CONTEXT(requireError("name path: a.b"));

        // now, test in which order the sections are tested.
        WITH_CONTEXT(requireFailLines({
            "[b]",
            "b = \"text\"",
            "a = \"text\"",
            "[a]",
            "b = \"text\"",
            "a = \"text\"",
            "[a.c]",
            "a = 3",
        }));
        // expect that "b.b" is failing first.
        WITH_CONTEXT(requireError("name path: b.b"));

        // now, test if each nested branch is tested completely before proceeding to the next root section.
        WITH_CONTEXT(requireFailLines({
            "[a]",
            "a = 1",
            "b = 2",
            "[b]",
            "a = \"text\"",
            "b = \"text\"",
            "[a.c]",
            "a = \"text\"",
        }));
        // expect that "a.c.a" is failing first, *before* errors in section "b"
        WITH_CONTEXT(requireError("name path: a.c.a"));
    }

    void testOrderOfAlternatives() {
        // Order of Alternatives: Alternatives must be evaluated in the order of their definition.
        // This allows rule authors to deterministically control which alternative applies when
        // multiple alternatives could match.
        WITH_CONTEXT(requireRulesPassLines({
            "*[server.bind]*",
            "type: \"text\"",
            "default: \"0.0.0.0:8080\"",
            "*[server.bind]*",
            "type: \"section\"",
            "[.address]",
            "type: \"text\"",
            "[.port]",
            "type: \"integer\"",
            "default: 8080",
            "*[server.bind]*",
            "type: \"section_list\"",
            "[.vr_entry.address]",
            "type: \"text\"",
            "[.vr_entry.port]",
            "type: \"integer\"",
        }));
        // As we have a default, no "bind" value should pass.
        WITH_CONTEXT(requirePassLines({
            "[server]",
        }));
        // Using a text value for the first alternative.
        WITH_CONTEXT(requirePassLines({
            "[server]",
            "bind: \"127.0.0.1:9000\"",
        }));
        // Using a section with "address" and "port" as the second alternative.
        WITH_CONTEXT(requirePassLines({
            "[server.bind]",
            "address: \"127.0.0.1\"",
            "port: 9000",
        }));
        // Using a section list for the third and last alternative.
        WITH_CONTEXT(requirePassLines({
            "*[server.bind]",
            "address: \"127.0.0.1\"",
            "port: 9000",
            "*[server.bind]",
            "address: \"192.168.1.1\"",
            "port: 8080",
            "*[server.bind]",
            "address: \"192.168.2.1\"",
            "port: 8080",
        }));
        // Other types must be rejected.
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "bind: 9000",
        }));
        // All possible types must be listed in order of definition.
        WITH_CONTEXT(requireError("Expected a text value, a section, or a section list but got an integer value"));
    }
};
