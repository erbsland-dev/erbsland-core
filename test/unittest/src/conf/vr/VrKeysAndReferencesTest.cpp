// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;

// Testing all requirements from chapter "Validation Rules"->"Keys and References".
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrKeysAndReferencesTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testIndexCreationAtRoot() {
        // Index Creation: An index is created by defining a section list named vr_key at the document root.
        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_key]*",
            "name: \"filter\"",
            "key: \"filter.vr_entry.identifier\"",
            "[filter]",
            "type: \"section_list\"",
            "[filter.vr_entry.identifier]",
            "type: \"text\"",
            "[app.start_filter]",
            "type: \"text\"",
            "key: \"filter\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[filter]*",
            "identifier: \"first\"",
            "[app]",
            "start_filter: \"first\"",
        }));
        // make sure the 'key' constraint is working.
        WITH_CONTEXT(requireFailLines({
            "*[filter]*",
            "identifier: \"first\"",
            "[app]",
            "start_filter: \"something else\"",
        }));
        WITH_CONTEXT(requireError("This value must refer to an existing key"));
    }

    void testIndexCreationInsideSection1() {
        // Placement: A vr_key section list may appear inside a node-rules definition for a section.
        WITH_CONTEXT(requireRulesPassLines({
            "[app.server]",
            "type: \"section_list\"",
            "[app.server.vr_entry.connection_id]",
            "type: \"text\"",
            "[app.server.vr_entry.protocol]",
            "type: \"text\"",
            "*[app.vr_key]*",
            "name: \"connection_id\"",
            "key: \"server.vr_entry.connection_id\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[app.server]*",
            "connection_id: \"primary\"",
            "protocol: \"https\"",
            "*[app.server]*",
            "connection_id: \"secondary\"",
            "protocol: \"ftps\"",
        }));
        // test a failing example to see if the index is created.
        WITH_CONTEXT(requireFailLines({
            "*[app.server]*",
            "connection_id: \"same\"",
            "protocol: \"https\"",
            "*[app.server]*",
            "connection_id: \"same\"", // error, duplicate
            "protocol: \"ftps\"",
        }));
        WITH_CONTEXT(requireError("Found a duplicate"));
    }

    void testIndexCreationInsideSection2() {
        // `vr_key` can also be placed in the section of a list.
        WITH_CONTEXT(requireRulesPassLines({
            "[a]",
            "type: \"section_list\"",
            "[a.vr_entry.b]",
            "type: \"section_list\"",
            "[a.vr_entry.b.vr_entry.c]",
            "type: \"integer\"",
            "*[a.vr_entry.vr_key]*",
            "key: \"b.vr_entry.c\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[a]*",
            "*[.b]*",
            "c: 1",
            "*[.b]*",
            "c: 2",
            "*[a]*",
            "*[.b]*",
            "c: 1", // valid, because entries are unique *locally to a*
            "*[.b]*",
            "c: 2",
        }));
        // Failure test to make sure the index works as expected.
        WITH_CONTEXT(requireFailLines({
            "*[a]*",
            "*[.b]*",
            "c: 1",
            "*[.b]*",
            "c: 2",
            "*[a]*",
            "*[.b]*",
            "c: 1",
            "*[.b]*",
            "c: 1", // error, duplicate
        }));
        WITH_CONTEXT(requireError("Found a duplicate"));
    }

    void testIndexUniqueness() {
        // Uniqueness: All key values collected in an index must be unique.
        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_key]*",
            "name: \"filter\"",
            "key: \"filter.vr_entry.identifier\"",
            "[filter]",
            "type: \"section_list\"",
            "[filter.vr_entry.identifier]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "*[filter]*",
            "identifier: \"one\"",
            "*[filter]*",
            "identifier: \"one\"",
        }));
        WITH_CONTEXT(requireError("Found a duplicate"));
    }

    void testIndexPlacementRestriction() {
        // Placement: vr_key is only allowed at the document root or inside a section rule.
        WITH_CONTEXT(requireRulesFailLines({
            "[app.start_filter]",
            "type: \"text\"",
            "*[app.start_filter.vr_key]*",
            "name: \"filter\"",
            "key: \"filter.vr_entry.identifier\"",
        }));
        WITH_CONTEXT(requireError("ConfKey definitions may only be placed in a section or the document root"));
    }

    void testIndexScopeAndVisibility() {
        // Scope and Visibility: An index is visible only within the subtree where it is defined.
        WITH_CONTEXT(requireRulesPassLines({
            "[a.b.c]",
            "type: \"section_list\"",
            "[a.b.c.vr_entry.d]",
            "type: \"integer\"",
            "*[a.b.vr_key]*",
            "name: \"index\"",
            "key: \"c.vr_entry.d\"",
            "[a.b.ref_d]",
            "type: \"integer\"",
            "key: \"index\"",
        }));
        WITH_CONTEXT(requireRulesFailLines({
            "[a.b.c]",
            "type: \"section_list\"",
            "[a.b.c.vr_entry.d]",
            "type: \"integer\"",
            "*[a.b.vr_key]*",
            "name: \"index\"",
            "key: \"c.vr_entry.d\"",
            "[a.ref_d]", // 'index' not visible from [a]
            "type: \"integer\"",
            "key: \"index\"",
        }));
        WITH_CONTEXT(requireError("was not found in the scope"));
    }

    void testKeyFieldRequired() {
        // ConfKey Field: Each vr_key entry must contain a key field.
        WITH_CONTEXT(requireRulesFailLines({
            "*[vr_key]*",
            "name: \"filter\"",
        }));
        WITH_CONTEXT(requireError("key"));
    }

    void testKeyFieldNamePathRequirement() {
        // Text Name-Path Required: key must be a text value with a valid name path.
        WITH_CONTEXT(requireRulesFailLines({
            "*[vr_key]*",
            "name: \"filter\"",
            "key: 123",
        }));
        WITH_CONTEXT(requireError("name path"));
    }

    void testAllowedKeyValueTypes() {
        // Allowed Value Types: A referenced key must point to a text or integer value.
        WITH_CONTEXT(requireRulesFailLines({
            "*[vr_key]*",
            "name: \"blog\"",
            "key: \"blog.vr_entry.created\"",
            "[blog]",
            "type: \"section_list\"",
            "[blog.vr_entry.created]",
            "type: \"date_time\"",
        }));
        WITH_CONTEXT(requireError("text"));
        WITH_CONTEXT(requireError("integer"));
    }

    void testSectionListValueRequirement() {
        // Section List + Value Requirement: key must resolve to a section list and a value within each entry.

        // failure: missing `vr_entry` in key.
        WITH_CONTEXT(requireRulesFailLines({
            "*[vr_key]*",
            "name: \"filter\"",
            "key: \"filter.identifier\"",
            "[filter]",
            "type: \"section\"",
            "[filter.identifier]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireError("section list"));
        // failure: a *section* list is required, no value list.
        WITH_CONTEXT(requireRulesFailLines({
            "*[vr_key]*",
            "name: \"filter\"",
            "key: \"filter.vr_entry.identifier\"",
            "[filter]",
            "type: \"value_list\"",
            "[filter.vr_entry.identifier]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireError("section list"));
    }

    void testCompositeKeyUniqueness() {
        // Composite Keys: The combination of multiple keys must be unique across all entries.
        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_key]*",
            "key:",
            "    * \"server.vr_entry.service\"",
            "    * \"server.vr_entry.protocol\"",
            "[server]",
            "type: \"section_list\"",
            "[server.vr_entry.service]",
            "type: \"text\"",
            "[server.vr_entry.protocol]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "*[server]*",
            "service: \"api\"",
            "protocol: \"https\"",
            "*[server]*",
            "service: \"api\"",
            "protocol: \"https\"",
        }));
        WITH_CONTEXT(requireError("unique"));
    }

    void testIndexNameRulesAndNormalization() {
        // Naming Rules: Index names must follow ELCL name rules.
        WITH_CONTEXT(requireRulesFailLines({
            "*[vr_key]*",
            "name: \"%my-name%\"",
            "key: \"filter.vr_entry.identifier\"",
        }));
        WITH_CONTEXT(requireError("name"));

        // Normalization and Comparison: Index names are normalized (case-insensitive, underscore/space equivalent).
        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_key]*",
            "name: \"filter_index\"",
            "key: \"filter.vr_entry.identifier\"",
            "[filter]",
            "type: \"section_list\"",
            "[filter.vr_entry.identifier]",
            "type: \"text\"",
            "[app.start_filter]",
            "type: \"text\"",
            "key: \"Filter Index\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[filter]*",
            "identifier: \"first\"",
            "[app]",
            "start_filter: \"first\"",
        }));
    }

    void testMultiKeyPartReference() {
        // Referencing Parts of a Multi-ConfKey: key[index] references a single part of a composite key.
        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_key]*",
            "name: \"server\"",
            "key: \"server.vr_entry.service\", \"server.vr_entry.protocol\"",
            "[server]",
            "type: \"section_list\"",
            "[server.vr_entry.service]",
            "type: \"text\"",
            "[server.vr_entry.protocol]",
            "type: \"text\"",
            "[network.ports]",
            "type: \"section_list\"",
            "[network.ports.vr_entry.protocol]",
            "type: \"text\"",
            "key: \"server[1]\"",
            "key_error: \"No server with this protocol was configured\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[server]*",
            "service: \"api\"",
            "protocol: \"https\"",
            "*[network.ports]*",
            "protocol: \"https\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "*[server]*",
            "service: \"api\"",
            "protocol: \"https\"",
            "*[network.ports]*",
            "protocol: \"ftp\"",
        }));
        WITH_CONTEXT(requireError("No server with this protocol was configured"));
    }

    void testKeyConstraintMultipleIndexesOrSemantics() {
        // Multiple Indexes (OR Semantics): Value is valid if it exists in any referenced index.
        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_key]*",
            "name: \"local_action\"",
            "key: \"local.vr_entry.id\"",
            "*[vr_key]*",
            "name: \"remote_action\"",
            "key: \"remote.vr_entry.id\"",
            "[local]",
            "type: \"section_list\"",
            "is_optional: true",
            "[local.vr_entry.id]",
            "type: \"text\"",
            "[remote]",
            "type: \"section_list\"",
            "is_optional: true",
            "[remote.vr_entry.id]",
            "type: \"text\"",
            "[app.start_action]",
            "type: \"text\"",
            "key: \"remote_action\", \"local_action\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[local]*",
            "id: \"local_1\"",
            "[app]",
            "start_action: \"local_1\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[remote]*",
            "id: \"remote_1\"",
            "[app]",
            "start_action: \"remote_1\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "*[local]*",
            "id: \"local_1\"",
            "*[remote]*",
            "id: \"remote_1\"",
            "[app]",
            "start_action: \"none\"",
        }));
        WITH_CONTEXT(requireError("existing key"));
    }

    void testCaseSensitivityDefinedByIndex() {
        // Case-Sensitivity: Index configuration controls duplicate detection and key matching.
        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_key]*",
            "name: \"filter\"",
            "key: \"filter.vr_entry.identifier\"",
            "case_sensitive: false",
            "[filter]",
            "type: \"section_list\"",
            "[filter.vr_entry.identifier]",
            "type: \"text\"",
            "[app.start_filter]",
            "type: \"text\"",
            "case_sensitive: true",
            "key: \"filter\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "*[filter]*",
            "identifier: \"Alpha\"",
            "*[filter]*",
            "identifier: \"alpha\"",
            "[app]",
            "start_filter: \"alpha\"",
        }));
        WITH_CONTEXT(requireError("duplicate"));

        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_key]*",
            "name: \"filter\"",
            "key: \"filter.vr_entry.identifier\"",
            "case_sensitive: true",
            "[filter]",
            "type: \"section_list\"",
            "[filter.vr_entry.identifier]",
            "type: \"text\"",
            "[app.start_filter]",
            "type: \"text\"",
            "case_sensitive: true",
            "key: \"filter\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[filter]*",
            "identifier: \"Alpha\"",
            "*[filter]*",
            "identifier: \"alpha\"",
            "[app]",
            "start_filter: \"Alpha\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "*[filter]*",
            "identifier: \"Alpha\"",
            "[app]",
            "start_filter: \"ALPHA\"",
        }));
        WITH_CONTEXT(requireError("existing key"));
    }

    void testNearestAncestorIndexResolution() {
        // Nearest Ancestor Resolution: Use the nearest matching index name in the same/ancestor branch.
        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_key]*",
            "name: \"id\"",
            "key: \"global_filters.vr_entry.id\"",
            "[global_filters]",
            "type: \"section_list\"",
            "[global_filters.vr_entry.id]",
            "type: \"text\"",
            "[server]",
            "type: \"section\"",
            "[server.connections]",
            "type: \"section_list\"",
            "[server.connections.vr_entry.id]",
            "type: \"text\"",
            "*[server.vr_key]*",
            "name: \"id\"",
            "key: \"connections.vr_entry.id\"",
            "[server.main_connection]",
            "type: \"text\"",
            "key: \"id\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[global_filters]*",
            "id: \"global_1\"",
            "[server]",
            "main_connection: \"server_1\"",
            "*[.connections]*",
            "id: \"server_1\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "*[global_filters]*",
            "id: \"global_1\"",
            "[server]",
            "main_connection: \"global_1\"",
            "*[.connections]*",
            "id: \"server_1\"",
        }));
        WITH_CONTEXT(requireError("existing key"));
    }

    void testCompositeKeysMustPointToSingleSectionList() {
        // All Keys Must Reference the Same Section List.
        WITH_CONTEXT(requireRulesFailLines({
            "*[vr_key]*",
            "name: \"mixed\"",
            "key: \"server.vr_entry.service\", \"client.vr_entry.protocol\"",
            "[server]",
            "type: \"section_list\"",
            "[server.vr_entry.service]",
            "type: \"text\"",
            "[client]",
            "type: \"section_list\"",
            "[client.vr_entry.protocol]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireError("same section list"));
    }

    void testCompositeKeyWholeReferenceCommaRepresentation() {
        // Composite ConfKey Representation: Whole-key references use comma-separated value tuples.
        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_key]*",
            "name: \"server\"",
            "key: \"server.vr_entry.service\", \"server.vr_entry.protocol\"",
            "[server]",
            "type: \"section_list\"",
            "[server.vr_entry.service]",
            "type: \"text\"",
            "[server.vr_entry.protocol]",
            "type: \"text\"",
            "[network.start]",
            "type: \"text\"",
            "key: \"server\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[server]*",
            "service: \"api\"",
            "protocol: \"https\"",
            "[network]",
            "start: \"api,https\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "*[server]*",
            "service: \"api\"",
            "protocol: \"https\"",
            "[network]",
            "start: \"api\"",
        }));
        WITH_CONTEXT(requireError("existing key"));
    }

    void testNestedSectionListPathIsRejected() {
        // No Nested Section Lists: ConfKey paths must not reference nested section lists.
        WITH_CONTEXT(requireRulesFailLines({
            "[app.filters]",
            "type: \"section_list\"",
            "[app.filters.vr_entry.rules]",
            "type: \"section_list\"",
            "[app.filters.vr_entry.rules.vr_entry.id]",
            "type: \"text\"",
            "*[vr_key]*",
            "key: \"app.filters.vr_entry.rules.vr_entry.id\"",
        }));
        WITH_CONTEXT(requireError("nested"));
    }

    void testMissingReferencedSectionListCreatesEmptyIndex() {
        // If a referenced section list is missing in the document, an empty index is created.
        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_key]*",
            "name: \"filter\"",
            "key: \"filter.vr_entry.id\"",
            "[filter]",
            "type: \"section_list\"",
            "is_optional: yes",
            "[filter.vr_entry.id]",
            "type: \"text\"",
            "[app.start]",
            "type: \"text\"",
            "key: \"filter\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "start: \"first\"",
        }));
        WITH_CONTEXT(requireError("existing key"));
    }

    void testMissingValuesAreOmittedFromIndex() {
        // Missing values in referenced key paths are omitted from the index.
        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_key]*",
            "name: \"filter\"",
            "key: \"filter.vr_entry.id\"",
            "[filter]",
            "type: \"section_list\"",
            "[filter.vr_entry.id]",
            "type: \"text\"",
            "is_optional: yes",
            "[app]",
            "type: \"section\"",
            "is_optional: yes",
            "[app.filter_reference]",
            "type: \"text\"",
            "is_optional: yes",
            "key: \"filter\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[filter]*",
            "*[filter]*",
            "*[filter]*",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[filter]*",
            "*[filter]*",
            "*[filter]*",
            "id: \"first\"",
            "[app]",
            "filter_reference: \"first\"",
        }));
        WITH_CONTEXT(requireFailLines({
            "*[filter]*",
            "*[filter]*",
            "*[filter]*",
            "[app]",
            "filter_reference: \"first\"",
        }));
    }

    void testVersionInactiveValuesAreOmittedFromIndex() {
        // ConfVersion-inactive values in referenced key paths are omitted from the index.
        WITH_CONTEXT(requireRulesPassLines({
            "*[vr_key]*",
            "name: \"filter\"",
            "key: \"filter.vr_entry.id\"",
            "[filter]",
            "type: \"section_list\"",
            "[filter.vr_entry.id]",
            "type: \"text\"",
            "minimum_version: 2",
        }));
        WITH_CONTEXT(requirePassLines({
            "*[filter]*",
            "*[filter]*",
            "*[filter]*",
        }));
    }

    void testKeyConstraintTypeAlignmentWithIndexValues() {
        // Type Alignment: Referencing node type must match stored key value type.
        WITH_CONTEXT(requireRulesFailLines({
            "*[vr_key]*",
            "name: \"filter\"",
            "key: \"filter.vr_entry.id\"",
            "[filter]",
            "type: \"section_list\"",
            "[filter.vr_entry.id]",
            "type: \"text\"",
            "[app.start_filter]",
            "type: \"integer\"",
            "key: \"filter\"",
        }));
        WITH_CONTEXT(requireError("type"));
    }
};
