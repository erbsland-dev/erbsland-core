// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringView.hpp"
#include "TextNodeData_fwd.hpp"

namespace erbsland::text {

/// Extensible metadata attached to a text-document node.
///
/// Applications may derive their own data types and attach them to nodes with TextNode::setData(). The string
/// representation is used by diagnostic trees.
/// @tested{TextDocumentTest}
class TextNodeData {
public:
    /// Destroy the metadata object.
    virtual ~TextNodeData() = 0;

public:
    /// Convert the metadata to a diagnostic string.
    [[nodiscard]] virtual auto toString() const -> StringView = 0;
};

}
