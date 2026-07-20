// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AssignmentStreamHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(AssignmentStream)
class AssignmentStreamTextNameTest final : public UNITTEST_SUBCLASS(AssignmentStreamHelper) {
public:
    void testTextNames() {
        WITH_CONTEXT(setupAssignmentStream("text_name_values.elcl"));
        WITH_CONTEXT(requireSectionMap("text_names"_el));
        WITH_CONTEXT(requireValue("text_names.\"One\""_el, ValueType::Integer, 1));
        WITH_CONTEXT(requireValue("text_names.\"  Two  \""_el, ValueType::Integer, 2));
        WITH_CONTEXT(requireValue(
            "text_names.\"Good Morning!\""_el, ValueType::Text, el::text::String{"おはようございます！"_el}));
        WITH_CONTEXT(requireValue("text_names.\"\\u{1f606}\""_el, ValueType::Text, el::text::String{"😆"_el}));
        WITH_CONTEXT(requireValue("text_names.\"->\\u{1f606}\""_el, ValueType::Text, el::text::String{"😆"_el}));
    }
};
