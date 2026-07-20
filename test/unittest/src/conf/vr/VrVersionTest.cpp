// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

using namespace el::conf;

// Testing the 'version', 'minimum_version' and 'maximum_version' constraints.
TESTED_TARGETS(VersionConstraint)
TAGS(ValidationRules)
class VrVersionTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testUsageInTypes() {
        const std::set<vr::RuleType> supportedTypes = {
            vr::RuleType::Integer,
            vr::RuleType::Boolean,
            vr::RuleType::Float,
            vr::RuleType::Text,
            vr::RuleType::Date,
            vr::RuleType::Time,
            vr::RuleType::DateTime,
            vr::RuleType::Bytes,
            vr::RuleType::TimeDelta,
            vr::RuleType::RegEx,
            vr::RuleType::Value,
            vr::RuleType::ValueList,
            vr::RuleType::ValueMatrix,
            vr::RuleType::Section,
            vr::RuleType::SectionList,
            vr::RuleType::SectionWithTexts,
            vr::RuleType::NotValidated};
        WITH_CONTEXT(requireConstraintValidForRuleTypes("version: 1", supportedTypes));
        WITH_CONTEXT(requireConstraintValidForRuleTypes("minimum_version: 1", supportedTypes));
        WITH_CONTEXT(requireConstraintValidForRuleTypes("maximum_version: 1", supportedTypes));
    }

    void testVersionConstraintDefinitionValues() {
        WITH_CONTEXT(requireOneConstraintPass("version: 0", vr::RuleType::Integer));
        WITH_CONTEXT(requireOneConstraintPass("version: 1, 2, 3", vr::RuleType::Integer));

        WITH_CONTEXT(requireOneConstraintFail("version: true", vr::RuleType::Integer));
        WITH_CONTEXT(requireError("one or more integers"));

        WITH_CONTEXT(requireOneConstraintFail("version: 1, \"2\"", vr::RuleType::Integer));
        WITH_CONTEXT(requireError("one or more integers"));

        WITH_CONTEXT(requireOneConstraintFail("version: -1", vr::RuleType::Integer));
        WITH_CONTEXT(requireError("must be non-negative integers"));

        WITH_CONTEXT(requireOneConstraintFail("version: 2, 2", vr::RuleType::Integer));
        WITH_CONTEXT(requireError("must be unique"));
    }

    void testMinimumAndMaximumDefinitionValues() {
        WITH_CONTEXT(requireOneConstraintPass("minimum_version: 0", vr::RuleType::Integer));
        WITH_CONTEXT(requireOneConstraintPass("maximum_version: 5", vr::RuleType::Integer));

        WITH_CONTEXT(requireOneConstraintFail("minimum_version: 1, 2", vr::RuleType::Integer));
        WITH_CONTEXT(requireError("The 'minimum_version' value must be an integer"));

        WITH_CONTEXT(requireOneConstraintFail("maximum_version: 1, 2", vr::RuleType::Integer));
        WITH_CONTEXT(requireError("The 'maximum_version' value must be an integer"));

        WITH_CONTEXT(requireOneConstraintFail("minimum_version: -1", vr::RuleType::Integer));
        WITH_CONTEXT(requireError("The 'minimum_version' value must be non-negative"));

        WITH_CONTEXT(requireOneConstraintFail("maximum_version: -1", vr::RuleType::Integer));
        WITH_CONTEXT(requireError("non-negative"));
    }

    void testVersionConstraintsUseAndSemantics() {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.x]",
            "type: \"integer\"",
            "version: 1, 2, 3",
            "minimum_version: 2",
            "maximum_version: 2",
        }));

        WITH_CONTEXT(requirePassLines(
            {
                "[app]",
            },
            1));

        WITH_CONTEXT(requireFailLines(
            {
                "[app]",
            },
            2));
        WITH_CONTEXT(requireError("expected an integer value with the name 'x'"));

        WITH_CONTEXT(requirePassLines(
            {
                "[app]",
            },
            3));
    }

    void testVersionConstraintsAreInclusive() {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.x]",
            "type: \"integer\"",
            "minimum_version: 2",
            "maximum_version: 4",
        }));

        WITH_CONTEXT(requireFailLines(
            {
                "[app]",
            },
            2));
        WITH_CONTEXT(requireError("expected an integer value with the name 'x'"));

        WITH_CONTEXT(requireFailLines(
            {
                "[app]",
            },
            4));
        WITH_CONTEXT(requireError("expected an integer value with the name 'x'"));

        WITH_CONTEXT(requirePassLines(
            {
                "[app]",
            },
            1));
        WITH_CONTEXT(requirePassLines(
            {
                "[app]",
            },
            5));
    }

    void testIgnoredDefinitionsAcrossVersions() {
        // Example from the docs: integer port for all versions, text ports from version 2 on.
        WITH_CONTEXT(requireRulesPassLines({
            "*[server.port]*",
            "type: \"integer\"",
            "minimum: 1",
            "maximum: 65534",
            "*[server.port]*",
            "type: \"text\"",
            "in: \"http\", \"https\", \"smtp\", \"smtps\"",
            "minimum_version: 2",
        }));

        WITH_CONTEXT(requirePassLines(
            {
                "[server]",
                "port: 8080",
            },
            1));

        WITH_CONTEXT(requireFailLines(
            {
                "[server]",
                "port: \"https\"",
            },
            1));
        WITH_CONTEXT(requireError("integer value"));

        WITH_CONTEXT(requirePassLines(
            {
                "[server]",
                "port: 8080",
            },
            2));

        WITH_CONTEXT(requirePassLines(
            {
                "[server]",
                "port: \"https\"",
            },
            2));
    }

    void testNegatedVersionConstraints() {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.x]",
            "type: \"integer\"",
            "not_version: 2",
        }));
        WITH_CONTEXT(requirePassLines(
            {
                "[app]",
            },
            2));
        WITH_CONTEXT(requireFailLines(
            {
                "[app]",
            },
            1));
        WITH_CONTEXT(requireError("name 'x'"));

        WITH_CONTEXT(requireRulesPassLines({
            "[app.x]",
            "type: \"integer\"",
            "not_minimum_version: 4",
        }));
        WITH_CONTEXT(requirePassLines(
            {
                "[app]",
            },
            4));
        WITH_CONTEXT(requireFailLines(
            {
                "[app]",
            },
            3));
        WITH_CONTEXT(requireError("name 'x'"));

        WITH_CONTEXT(requireRulesPassLines({
            "[app.x]",
            "type: \"integer\"",
            "not_maximum_version: 1",
        }));
        WITH_CONTEXT(requirePassLines(
            {
                "[app]",
            },
            1));
        WITH_CONTEXT(requireFailLines(
            {
                "[app]",
            },
            2));
        WITH_CONTEXT(requireError("name 'x'"));
    }

    void testNoCustomErrorMessagesForVersionConstraints() {
        WITH_CONTEXT(requireRulesFailLines({
            "[app.x]",
            "type: \"integer\"",
            "version: 1",
            "version_error: \"version failed\"",
        }));
        WITH_CONTEXT(requireError("'version' does not accept '_error' suffixes"));

        WITH_CONTEXT(requireRulesFailLines({
            "[app.x]",
            "type: \"integer\"",
            "minimum_version: 1",
            "minimum_version_error: \"minimum_version failed\"",
        }));
        WITH_CONTEXT(requireError("'minimum_version' does not accept '_error' suffixes"));

        WITH_CONTEXT(requireRulesFailLines({
            "[app.x]",
            "type: \"integer\"",
            "maximum_version: 1",
            "maximum_version_error: \"maximum_version failed\"",
        }));
        WITH_CONTEXT(requireError("'maximum_version' does not accept '_error' suffixes"));
    }
};
