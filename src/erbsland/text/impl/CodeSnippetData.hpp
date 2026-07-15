// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../StringView.hpp"
#include "../TextNodeData.hpp"

#include <utility>

namespace erbsland::text::impl {

/// Language metadata attached to a code-snippet node.
class CodeSnippetData final : public TextNodeData {
public:
    explicit CodeSnippetData(StringView language) noexcept : _language{std::move(language)} {}

public:
    [[nodiscard]] auto language() const noexcept -> StringView { return _language; }
    [[nodiscard]] auto toString() const -> StringView override { return _language; }

private:
    StringView _language; ///< The optional language identifier.
};

}
