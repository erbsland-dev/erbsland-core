// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;

// Testing all requirements from chapter "Validation Rules"->"Variable Names".
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrVariableNamesTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testBasicVrAny() {
        // Basic Use of `vr_any`: match any valid name or text-name.
        WITH_CONTEXT(requireRulesPassLines({
            "[user.vr_any]",
            "type: \"section\"",
            "[user.vr_any.age]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[user.alice]",
            "age: 32",
            "[user.benjamin]",
            "age: 48",
        }));
        WITH_CONTEXT(requireRulesPassLines({
            "[user]",
            "type: \"section_with_texts\"",
            "[user.vr_any]",
            "type: \"section\"",
            "[user.vr_any.age]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[user.\"charlotte brown\"]",
            "age: 56",
        }));
    }

    void testVrAnyRequiredForTextNames() {
        // Text Names Allowed: vr_any can be used to validate text-names, which are intended by ELCL to be used as keys.
        // This makes vr_any the only mechanism that allows validation of nodes addressed by text-names.
        WITH_CONTEXT(requireRulesPassLines({
            "[user]",
            "type: \"section_with_texts\"",
            "[user.age]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[user.alice]",
            "\"age\": 32",
        }));
    }

    void testNoDefault() {
        WITH_CONTEXT(requireRulesFailLines({
            "[user.vr_any]",
            "type: \"integer\"",
            "default: 42",
        }));
        WITH_CONTEXT(requireError("The 'vr_any' rule cannot have a default"));
    }

    void testNoOptional() {
        WITH_CONTEXT(requireRulesFailLines({
            "[user.vr_any]",
            "type: \"integer\"",
            "is_optional: yes",
        }));
        WITH_CONTEXT(requireError("The 'vr_any' rule cannot be set optional"));
    }

    void testZeroToMany() {
        WITH_CONTEXT(requireRulesPassLines({
            "[user.vr_any]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[user]",
        }));
        WITH_CONTEXT(requirePassLines({
            "[user]",
            "a: 1",
        }));
        WITH_CONTEXT(requirePassLines({
            "[user]",
            "a: 1",
            "b: 2",
            "c: 3",
        }));
        WITH_CONTEXT(requireRulesPassLines({
            "[user]",
            "type: \"section\"",
            "minimum: 1",
            "maximum: 2",
            "[user.vr_any]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[user]",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section must be at least 1"));
        WITH_CONTEXT(requirePassLines({
            "[user]",
            "a: 1",
        }));
        WITH_CONTEXT(requireFailLines({
            "[user]",
            "a: 1",
            "b: 2",
            "c: 3",
        }));
        WITH_CONTEXT(requireError("The number of entries in this section must be at most 2"));
    }

    void testVariableNamesAsValues() {
        // Variable Names as Values: use `vr_any` as value entries.
        WITH_CONTEXT(requireRulesPassLines({
            "[tags]",
            "type: \"section\"",
            "[tags.vr_any]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[tags]",
            "feature: 1",
            "bugfix: 2",
            "enhancement: 3",
        }));
    }

    void testVrNameConstraints() {
        // Optional Name Constraints: define `vr_name` to restrict variable names.
        WITH_CONTEXT(requireRulesPassLines({
            "[user.vr_any]",
            "type: \"section\"",
            "[user.vr_any.vr_name]",
            "starts: \"u_\"",
            "[user.vr_any.age]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[user.u_alice]",
            "age: 32",
            "[user.u_benjamin]",
            "age: 48",
        }));
        WITH_CONTEXT(requireFailLines({
            "[user.alice]",
            "age: 32",
        }));
        WITH_CONTEXT(requireError("Value name validation failed:"));
    }

    void testVrNameMustBeText() {
        WITH_CONTEXT(requireRulesPassLines({
            "[user.vr_any]",
            "type: \"integer\"",
            "[user.vr_any.vr_name]",
        }));
        WITH_CONTEXT(requireRulesPassLines({
            "[user.vr_any]",
            "type: \"integer\"",
            "[user.vr_any.vr_name]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireRulesFailLines({
            "[user.vr_any]",
            "type: \"integer\"",
            "[user.vr_any.vr_name]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireError("Name node-rules must have a 'type' value of 'text'"));
    }

    void testVrNameConstraintsApplyToNameOnly() {
        // Scope: `vr_name` constraints only apply to the name, not the value.
        WITH_CONTEXT(requireRulesPassLines({
            "[user.vr_any]",
            "type: \"text\"",
            "[user.vr_any.vr_name]",
            "starts: \"u_\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[user]",
            "u_alice: \"not starting with u_\"",
            "u_benjamin: \"no_prefix_here\"",
        }));
    }

    void testVrNameTextSemantics() {
        // Text Semantics: `vr_name` supports text constraints and case sensitivity.
        WITH_CONTEXT(requireRulesPassLines({
            "[user]",
            "type: \"section_with_texts\"",
            "[user.vr_any]",
            "type: \"integer\"",
            "[user.vr_any.vr_name]",
            "case_sensitive: yes",
            "starts: \"u_\"",
            "ends: \"_x\"",
            "chars: \"(a-z)\", \"[_]\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[user]",
            "\"u_alpha_x\": 1",
        }));
        WITH_CONTEXT(requireFailLines({
            "[user]",
            "\"U_alpha_x\": 1",
        }));
        WITH_CONTEXT(requireError("Value name validation failed:"));
        WITH_CONTEXT(requireFailLines({
            "[user]",
            "\"user10\": 1",
        }));
        WITH_CONTEXT(requireError("Value name validation failed:"));
    }
};
