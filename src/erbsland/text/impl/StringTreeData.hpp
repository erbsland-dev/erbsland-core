// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../String.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace erbsland::text::impl {

struct StringTreeData;
using StringTreeDataPtr = std::shared_ptr<StringTreeData>;

enum class StringTreeEntryKind {
    Text,
    Value,
    Tree,
};

struct StringTreeEntry final {
    StringTreeEntryKind kind{StringTreeEntryKind::Text};
    String label;
    String value;
    StringTreeDataPtr treeData;

    [[nodiscard]] static auto text(String value) -> StringTreeEntry {
        return StringTreeEntry{
            .kind = StringTreeEntryKind::Text,
            .label = {},
            .value = std::move(value),
            .treeData = {},
        };
    }

    [[nodiscard]] static auto labeledValue(String label, String value) -> StringTreeEntry {
        return StringTreeEntry{
            .kind = StringTreeEntryKind::Value,
            .label = std::move(label),
            .value = std::move(value),
            .treeData = {},
        };
    }

    [[nodiscard]] static auto tree(String label, StringTreeDataPtr treeData) -> StringTreeEntry {
        return StringTreeEntry{
            .kind = StringTreeEntryKind::Tree,
            .label = std::move(label),
            .value = {},
            .treeData = std::move(treeData),
        };
    }
};

struct StringTreeData final {
    String title;
    std::vector<StringTreeEntry> entries;
};

}
