// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;
using namespace el::text::literals;

// Testing all requirements from chapter "Validation Rules"->"Defaults and Optionality".
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrDefaultsAndOptionalityTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testDefaultAppliedToMissingScalarAndList() {
        // Default Field: A node-rules definition may specify a default value that is used if the node is missing.
        // Allowed Node Types: Defaults may only be defined for scalar values and value lists.
        WITH_CONTEXT(requireRulesPassLines({
            "[api]",
            "type: \"section\"",
            "[api.host]",
            "type: \"text\"",
            "default: \"127.0.0.1\"",
            "[api.tags]",
            "type: \"value_list\"",
            "default: \"article\", \"news\"",
            "[api.tags.vr_entry]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[api]",
        }));
        REQUIRE_EQUAL(document->getTextOrThrow("api.host"_el), "127.0.0.1"_el);
        const auto tags = document->getValueListOrThrow("api.tags"_el);
        REQUIRE_EQUAL(tags.size(), 2U);
        REQUIRE_EQUAL(tags[0]->asText(), "article"_el);
        REQUIRE_EQUAL(tags[1]->asText(), "news"_el);
    }

    void testDefaultTypeMatching() {
        // Type Matching: A default value must match the declared type of the node-rules definition.
        WITH_CONTEXT(requireRulesFailLines({
            "[api.port]",
            "type: \"integer\"",
            "default: \"text\"",
        }));
        WITH_CONTEXT(requireError("The 'default' value must be an integer value"));
    }

    void testDefaultIgnoredConstraints() {
        // Constraints Ignored: Default values are not validated against local constraints (except type).
        WITH_CONTEXT(requireRulesPassLines({
            "[server]",
            "type: \"section\"",
            "[server.name]",
            "type: \"text\"",
            "minimum: 1",
            "default: \"\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[server]",
        }));
        REQUIRE_EQUAL(document->getTextOrThrow("server.name"_el), ""_el);

        // If the node exists, constraints must apply.
        WITH_CONTEXT(requireFailLines({
            "[server]",
            "name: \"\"",
        }));
        WITH_CONTEXT(requireError("The number of characters in this text must be at least 1"));
    }

    void testDefaultsOnlyForScalarAndValueLists() {
        // Allowed Node Types: Defaults may only be defined for scalar values and value lists.
        WITH_CONTEXT(requireRulesFailLines({
            "[client]",
            "type: \"section\"",
            "default: 1",
        }));
        WITH_CONTEXT(requireError("A default value cannot be used for 'Section' node rules"));
    }

    void testNoDefaultCombinedWithOptional() {
        // No Combination with Optionality: default must not be combined with is_optional.
        WITH_CONTEXT(requireRulesFailLines({
            "[server.name]",
            "type: \"text\"",
            "is_optional: yes",
            "default: \"example\"",
        }));
        WITH_CONTEXT(requireError("may not be both optional and have a default value"));
    }

    void testOptionalityIncludesChildNodes() {
        // Optionality Includes Child Nodes: If an optional node does not exist, its child nodes are ignored.
        WITH_CONTEXT(requireRulesPassLines({
            "[client]",
            "type: \"section\"",
            "is_optional: yes",
            "[client.name]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requirePassLines({}));

        // If the optional parent exists, child nodes must be validated.
        WITH_CONTEXT(requireFailLines({
            "[client]",
        }));
        WITH_CONTEXT(requireError("expected a text value with the name 'name'"));
    }

    void testNoDefaultsForMissingOptionalParent() {
        // No Defaults for Missing Optional Nodes: Defaults are not applied if an optional parent is missing.
        WITH_CONTEXT(requireRulesPassLines({
            "[client]",
            "type: \"section\"",
            "is_optional: yes",
            "[client.name]",
            "type: \"text\"",
            "default: \"unknown\"",
        }));
        WITH_CONTEXT(requirePassLines({}));
        REQUIRE_FALSE(document->hasValue(el::text::String{"client"}));
        REQUIRE_FALSE(document->hasValue(el::text::String{"client.name"}));

        // If the optional parent exists, defaults are applied to missing child nodes.
        WITH_CONTEXT(requirePassLines({
            "[client]",
        }));
        REQUIRE_EQUAL(document->getTextOrThrow("client.name"_el), "unknown"_el);
    }
};
