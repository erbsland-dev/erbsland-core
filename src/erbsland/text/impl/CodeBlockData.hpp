// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../String.hpp"
#include "../TextNodeData.hpp"

#include <utility>

namespace erbsland::text::impl {

/// Language metadata attached to a code-block node.
class CodeBlockData final : public TextNodeData {
public:
    /// Create code-block metadata with its optional `language` identifier.
    explicit CodeBlockData(String language) noexcept : _language{std::move(language)} {}

public:
    /// Get the optional programming-language identifier.
    [[nodiscard]] auto language() const noexcept -> String { return _language; }

public: // implement TextNodeData
    [[nodiscard]] auto toString() const -> String override { return _language; }

private:
    String _language; ///< The optional language identifier.
};

}
