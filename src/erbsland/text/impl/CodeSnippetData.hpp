// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../String.hpp"
#include "../TextNodeData.hpp"

#include <utility>

namespace erbsland::text::impl {

/// Language metadata attached to a code-snippet node.
class CodeSnippetData final : public TextNodeData {
public:
    explicit CodeSnippetData(String language) noexcept : _language{std::move(language)} {}

public:
    [[nodiscard]] auto language() const noexcept -> String { return _language; }
    [[nodiscard]] auto toString() const -> String override { return _language; }

private:
    String _language; ///< The optional language identifier.
};

}
