// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AssignmentStreamHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(AssignmentStream)
class AssignmentStreamSectionListTest final : public UNITTEST_SUBCLASS(AssignmentStreamHelper) {
public:
    void testSectionLists() {
        WITH_CONTEXT(setupAssignmentStream("section_lists.elcl"));
        WITH_CONTEXT(requireSectionList("server"_el));
        WITH_CONTEXT(requireValue("server.value"_el, ValueType::Integer, 1));
        WITH_CONTEXT(requireSectionList("server"_el));
        WITH_CONTEXT(requireValue("server.value"_el, ValueType::Integer, 2));
        WITH_CONTEXT(requireSectionList("server"_el));
        WITH_CONTEXT(requireValue("server.value"_el, ValueType::Integer, 3));
        WITH_CONTEXT(requireSectionList("client.config"_el));
        WITH_CONTEXT(requireValue("client.config.value"_el, ValueType::Integer, 1));
        WITH_CONTEXT(requireSectionList("client.config"_el));
        WITH_CONTEXT(requireValue("client.config.value"_el, ValueType::Integer, 2));
        WITH_CONTEXT(requireSectionList("client.config"_el));
        WITH_CONTEXT(requireValue("client.config.value"_el, ValueType::Integer, 3));
    }
};
