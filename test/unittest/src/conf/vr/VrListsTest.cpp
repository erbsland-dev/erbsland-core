// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;

// Testing all requirements from chapter "Validation Rules"->"Lists".
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrListsTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testListLevelAndEntryLevelConstraints() {
        // List-level constraints apply to the list as a whole, while vr_entry rules apply to each entry.
        WITH_CONTEXT(requireRulesPassLines({
            "[app.tags]",
            "type: \"value_list\"",
            "minimum: 1",
            "maximum: 3",
            "[app.tags.vr_entry]",
            "type: \"text\"",
            "minimum: 2",
            "maximum: 5",
        }));
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "tags: \"aa\", \"bbb\"",
        }));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "tags: \"aa\", \"bbb\", \"cccc\", \"dd\"",
        }));
        WITH_CONTEXT(requireError("at most"));

        WITH_CONTEXT(requireFailLines({
            "[app]",
            "tags: \"a\"",
        }));
        WITH_CONTEXT(requireError("at least"));
    }

    void testVrEntryAlternativesAllowed() {
        // A vr_entry definition may be a section list to define alternatives for list entries.
        WITH_CONTEXT(requireRulesPassLines({
            "[ruler.marks]",
            "type: \"value_list\"",
            "maximum: 4",
            "*[.vr_entry]*",
            "type: \"integer\"",
            "*[.vr_entry]*",
            "type: \"float\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[ruler]",
            "marks: 1, 2.5, 3",
        }));
        WITH_CONTEXT(requireFailLines({
            "[ruler]",
            "marks: \"bad\"",
        }));
        WITH_CONTEXT(requireError("Expected"));
    }

    void testValueListAndMatrixRequireVrEntry() {
        // Value lists and matrices must declare a vr_entry subsection.
        WITH_CONTEXT(requireRulesFailLines({
            "[app.tags]",
            "type: \"value_list\"",
        }));
        WITH_CONTEXT(requireError("vr_entry"));

        WITH_CONTEXT(requireRulesFailLines({
            "[app.matrix]",
            "type: \"value_matrix\"",
        }));
        WITH_CONTEXT(requireError("vr_entry"));
    }

    void testValueListAndMatrixEntryMustBeScalar() {
        // vr_entry for value lists and matrices is limited to scalar values.
        WITH_CONTEXT(requireRulesFailLines({
            "[app.tags]",
            "type: \"value_list\"",
            "[app.tags.vr_entry]",
            "type: \"section\"",
        }));
        WITH_CONTEXT(requireError("Unexpected 'vr_entry' node-rules definition type"));

        WITH_CONTEXT(requireRulesFailLines({
            "[app.matrix]",
            "type: \"value_matrix\"",
            "[app.matrix.vr_entry]",
            "type: \"section_list\"",
        }));
        WITH_CONTEXT(requireError("Unexpected 'vr_entry' node-rules definition type"));
    }

    void testDefaultsAreNotAllowed() {
        WITH_CONTEXT(requireRulesFailLines({
            "[app.tags]",
            "type: \"value_list\"",
            "[app.tags.vr_entry]",
            "type: \"integer\"",
            "default: 1",
        }));
        WITH_CONTEXT(requireError("The `vr_entry` node-rules definition may not have a default value"));
    }

    void testOptionalIsNotAllowed() {
        WITH_CONTEXT(requireRulesFailLines({
            "[app.tags]",
            "type: \"value_list\"",
            "[app.tags.vr_entry]",
            "type: \"integer\"",
            "is_optional: true",
        }));
        WITH_CONTEXT(requireError("The `vr_entry` node-rules definition cannot be optional"));
    }

    void testOnlyVrEntrySubNodeIsAllowed() {
        WITH_CONTEXT(requireRulesFailLines({
            "[app.tags]",
            "type: \"value_list\"",
            "[app.tags.vr_entry]",
            "type: \"integer\"",
            "[app.tags.name]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireError("only 'vr_entry' is permitted"));

        WITH_CONTEXT(requireRulesFailLines({
            "[app.tags]",
            "type: \"value_list\"",
            "[app.tags.vr_entry]",
            "type: \"integer\"",
            "[app.tags.vr_name]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireError("only 'vr_entry' is permitted"));

        WITH_CONTEXT(requireRulesFailLines({
            "[app.tags]",
            "type: \"value_list\"",
            "[app.tags.vr_entry]",
            "type: \"integer\"",
            "[app.tags.vr_any]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireError("only 'vr_entry' is permitted"));
    }

    void testSectionListRules() {
        // Section lists must define vr_entry with a section or section_with_texts type.
        WITH_CONTEXT(requireRulesFailLines({
            "[app.user]",
            "type: \"section_list\"",
        }));
        WITH_CONTEXT(requireError("vr_entry"));

        WITH_CONTEXT(requireRulesFailLines({
            "[app.user]",
            "type: \"section_list\"",
            "[app.user.vr_entry]",
            "type: \"integer\"",
        }));
        WITH_CONTEXT(requireError("The 'vr_entry' node-rules definition for a section list"));

        WITH_CONTEXT(requireRulesPassLines({
            "[app.user]",
            "type: \"section_list\"",
            "maximum: 2",
            "[app.user.vr_entry.name]",
            "type: \"text\"",
            "[app.user.vr_entry.email]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[app.user]*",
            "name: \"Example User 1\"",
            "email: \"user1@example.com\"",
            "*[app.user]*",
            "name: \"Example User 2\"",
            "email: \"user2@example.com\"",
        }));

        WITH_CONTEXT(requireFailLines({
            "*[app.user]*",
            "name: \"Example User 1\"",
        }));
        WITH_CONTEXT(requireError("email"));

        WITH_CONTEXT(requireFailLines({
            "*[app.user]*",
            "name: \"Example User 1\"",
            "email: \"user1@example.com\"",
            "*[app.user]*",
            "name: \"Example User 2\"",
            "email: \"user2@example.com\"",
            "*[app.user]*",
            "name: \"Example User 3\"",
            "email: \"user3@example.com\"",
        }));
        WITH_CONTEXT(requireError("at most"));

        WITH_CONTEXT(requireRulesPassLines({
            "[app.notes]",
            "type: \"section_list\"",
            "[app.notes.vr_entry]",
            "type: \"section_with_texts\"",
        }));
    }

    void testNestedList() {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.filters]",
            "type: \"SectionList\"",
            "[app.filters.vr_entry.rules]",
            "type: \"SectionList\"",
            "[app.filters.vr_entry.rules.vr_entry.id]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[app.filters]*",
            "*[.rules]*",
            "id: \"filter1\"",
            "*[.rules]*",
            "id: \"filter2\"",
            "*[app.filters]*",
            "*[.rules]*",
            "id: \"filter3\"",
            "*[.rules]*",
            "id: \"filter4\"",
        }));
    }
};
