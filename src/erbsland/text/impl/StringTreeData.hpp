// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../StringView.hpp"

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
    StringView label;
    StringView value;
    StringTreeDataPtr treeData;

    [[nodiscard]] static auto text(StringView value) -> StringTreeEntry {
        return StringTreeEntry{
            .kind = StringTreeEntryKind::Text,
            .label = {},
            .value = std::move(value),
            .treeData = {},
        };
    }

    [[nodiscard]] static auto labeledValue(StringView label, StringView value) -> StringTreeEntry {
        return StringTreeEntry{
            .kind = StringTreeEntryKind::Value,
            .label = std::move(label),
            .value = std::move(value),
            .treeData = {},
        };
    }

    [[nodiscard]] static auto tree(StringView label, StringTreeDataPtr treeData) -> StringTreeEntry {
        return StringTreeEntry{
            .kind = StringTreeEntryKind::Tree,
            .label = std::move(label),
            .value = {},
            .treeData = std::move(treeData),
        };
    }
};

struct StringTreeData final {
    StringView title;
    std::vector<StringTreeEntry> entries;
};

}
