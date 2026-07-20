// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

using namespace el::conf;

// Testing the 'minimum' constraint.
TESTED_TARGETS(MinimumConstraint)
TAGS(ValidationRules)
class VrMinimumTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testUsageInTypes() {
        // Integer-based minimum.
        WITH_CONTEXT(requireConstraintValidForRuleTypes(
            "minimum: 3",
            {vr::RuleType::Integer,
                vr::RuleType::Text,
                vr::RuleType::Bytes,
                vr::RuleType::ValueList,
                vr::RuleType::Section,
                vr::RuleType::SectionList,
                vr::RuleType::SectionWithTexts}));
        // Float minimum.
        WITH_CONTEXT(requireConstraintValidForRuleTypes("minimum: 0.25", {vr::RuleType::Float}));
        // el::time::Date minimum.
        WITH_CONTEXT(requireConstraintValidForRuleTypes("minimum: 2026-01-10", {vr::RuleType::Date}));
        // el::time::Date-time minimum.
        WITH_CONTEXT(requireConstraintValidForRuleTypes("minimum: 2026-01-10 12:30:00", {vr::RuleType::DateTime}));
        // Matrix minimum (rows, columns).
        WITH_CONTEXT(requireConstraintValidForRuleTypes("minimum: 2, 3", {vr::RuleType::ValueMatrix}));
    }

    void testUnsupportedValueTypes() {
        WITH_CONTEXT(requireOneConstraintFail("minimum: true", vr::RuleType::Integer));
        WITH_CONTEXT(requireError("must be of the type Integer"));
        WITH_CONTEXT(requireOneConstraintFail("minimum: 3", vr::RuleType::Float));
        WITH_CONTEXT(requireError("must be of the type Float"));
        WITH_CONTEXT(requireOneConstraintFail("minimum: 2026-01-10", vr::RuleType::DateTime));
        WITH_CONTEXT(requireError("must be of the type DateTime"));
        WITH_CONTEXT(requireOneConstraintFail("minimum: 2026-01-10 12:30:00", vr::RuleType::Date));
        WITH_CONTEXT(requireError("must be of the type Date"));
    }

    void testMinimumForInteger() {
        WITH_CONTEXT(requireOneConstraintPass("minimum: 42", vr::RuleType::Integer));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 42",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 99",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 41",
        }));
        WITH_CONTEXT(requireError("The value must be at least 42"));
    }

    void testNotMinimumForInteger() {
        WITH_CONTEXT(requireOneConstraintPass("not_minimum: 42", vr::RuleType::Integer));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 41",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 42",
        }));
        WITH_CONTEXT(requireError("The value must be less than 42"));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 100",
        }));
        WITH_CONTEXT(requireError("The value must be less than 42"));
    }

    void testMinimumErrorAndNotMinimumError() {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.a]",
            "type: \"integer\"",
            "minimum: 10",
            "minimum_error: \"minimum failed\"",
            "[app.b]",
            "type: \"integer\"",
            "not_minimum: 10",
            "not_minimum_error: \"not_minimum failed\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "a: 9",
            "b: 1",
        }));
        WITH_CONTEXT(requireError("minimum failed"));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "a: 10",
            "b: 10",
        }));
        WITH_CONTEXT(requireError("not_minimum failed"));
    }

    void testMinimumForTextAndBytes() {
        WITH_CONTEXT(requireOneConstraintPass("minimum: 2", vr::RuleType::Text));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"ab\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: \"äb\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: \"ä\"",
        }));
        WITH_CONTEXT(requireError("The number of characters in this text must be at least 2"));

        WITH_CONTEXT(requireOneConstraintPass("minimum: 2", vr::RuleType::Bytes));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: <01 02>",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: <01>",
        }));
        WITH_CONTEXT(requireError("The number of bytes must be at least 2"));
    }

    void testMinimumForCollectionsAndSections() {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.list]",
            "type: \"value_list\"",
            "minimum: 2",
            "[app.list.vr_entry]",
            "type: \"integer\"",
            "[app.groups]",
            "type: \"section_list\"",
            "minimum: 2",
            "[app.groups.vr_entry.id]",
            "type: \"integer\"",
            "[app.named]",
            "type: \"section\"",
            "minimum: 2",
            "[app.named.vr_any]",
            "type: \"integer\"",
            "[app.textual]",
            "type: \"section_with_texts\"",
            "minimum: 2",
            "[app.textual.vr_any]",
            "type: \"integer\"",
        }));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "list: 1, 2",
            "*[.groups]*",
            "id: 1",
            "*[.groups]*",
            "id: 2",
            "[.named]",
            "a: 1",
            "b: 2",
            "[.textual]",
            "\"aa\": 1",
            "\"bb\": 2",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "list: 1",
            "*[.groups]*",
            "id: 1",
            "[.named]",
            "a: 1",
            "[.textual]",
            "\"aa\": 1",
        }));
        WITH_CONTEXT(requireError("at least 2"));
    }

    void testMinimumForFloatAndSpecialValues() {
        WITH_CONTEXT(requireOneConstraintPass("minimum: 0.5", vr::RuleType::Float));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: 0.5",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x: inf",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: 0.49",
        }));
        WITH_CONTEXT(requireError("The value must be at least 0.5"));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: -inf",
        }));
        WITH_CONTEXT(requireError("The value must be at least 0.5"));
        // The documentation defines NaN as invalid when a bound exists.
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "x: NaN",
        }));
        WITH_CONTEXT(requireError("must be at least"));
    }

    void testMinimumForDateAndDateTime() {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.date_value]",
            "type: \"date\"",
            "minimum: 2026-01-10",
            "[app.datetime_value]",
            "type: \"datetime\"",
            "minimum: 2026-01-10 12:00:00",
        }));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "date_value: 2026-01-10",
            "datetime_value: 2026-01-10 12:00:00",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "date_value: 2026-01-11",
            "datetime_value: 2026-01-11 00:00:00",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "date_value: 2026-01-09",
            "datetime_value: 2026-01-11 00:00:00",
        }));
        WITH_CONTEXT(requireError("The date must be at least 2026-01-10"));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "date_value: 2026-01-10",
            "datetime_value: 2026-01-10 00:00:00",
        }));
        WITH_CONTEXT(requireError("The date-time must be at least 2026-01-10 12:00:00"));
    }

    void testMinimumForValueMatrix() {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.m]",
            "type: \"value_matrix\"",
            "minimum: 2, 3",
            "[app.m.vr_entry]",
            "type: \"integer\"",
        }));

        WITH_CONTEXT(requirePassLines({
            "[app]",
            "m:",
            "    * 1, 2, 3",
            "    * 4, 5, 6",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "m: 1, 2, 3",
        }));
        WITH_CONTEXT(requireError("The number of columns in this row must be at least 3"));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "m:",
            "    * 1, 2, 3",
            "    * 4",
        }));
        WITH_CONTEXT(requireError("The number of columns in this row must be at least 3"));

        WITH_CONTEXT(requireRulesPassLines({
            "[app.m]",
            "type: \"value_matrix\"",
            "minimum: 4, 1",
            "[app.m.vr_entry]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "m: 1, 2, 3",
        }));
        WITH_CONTEXT(requireError("The number of rows in this value matrix must be at least 4"));
    }

    void testMatrixMinimumDefinitionMustBeTwoIntegers() {
        WITH_CONTEXT(requireOneConstraintFail("minimum: 2", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(requireError("must be a list with two integer values"));
        WITH_CONTEXT(requireOneConstraintFail("minimum: 2, 3, 4", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(requireError("must be a list with two integer values"));
        WITH_CONTEXT(requireOneConstraintFail("minimum: \"2\", \"3\"", vr::RuleType::ValueMatrix));
        WITH_CONTEXT(requireError("must be a list with two integer values"));
    }

    void testMinimumMustNotExceedMaximum() {
        // The specification requires minimum <= maximum.
        WITH_CONTEXT(requireRulesFailLines({
            "[app.x]",
            "type: \"integer\"",
            "minimum: 100",
            "maximum: 10",
        }));
        WITH_CONTEXT(requireError("minimum"));
        WITH_CONTEXT(requireError("maximum"));
    }
};
