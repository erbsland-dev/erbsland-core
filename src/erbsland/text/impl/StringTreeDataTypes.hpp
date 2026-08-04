// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringTreeData_fwd.hpp"

#include "../String.hpp"

#include <utility>
#include <vector>

namespace erbsland::text::impl {

/// Identifies the content type of a string-tree entry.
enum class StringTreeEntryKind {
    Text,
    Value,
    Tree,
};

/// Stores one text, value, or child-tree entry.
struct StringTreeEntry final {
    StringTreeEntryKind kind{StringTreeEntryKind::Text};
    String label;
    String value;
    StringTreeDataPtr treeData;

    /// Create a text entry.
    [[nodiscard]] static auto text(String value) -> StringTreeEntry {
        return StringTreeEntry{
            .kind = StringTreeEntryKind::Text,
            .label = {},
            .value = std::move(value),
            .treeData = {},
        };
    }

    /// Create a labeled value entry.
    [[nodiscard]] static auto labeledValue(String label, String value) -> StringTreeEntry {
        return StringTreeEntry{
            .kind = StringTreeEntryKind::Value,
            .label = std::move(label),
            .value = std::move(value),
            .treeData = {},
        };
    }

    /// Create a labeled child-tree entry.
    [[nodiscard]] static auto tree(String label, StringTreeDataPtr treeData) -> StringTreeEntry {
        return StringTreeEntry{
            .kind = StringTreeEntryKind::Tree,
            .label = std::move(label),
            .value = {},
            .treeData = std::move(treeData),
        };
    }
};

/// Stores the title and entries of a string tree.
struct StringTreeData final {
    String title;
    std::vector<StringTreeEntry> entries;
};

}
