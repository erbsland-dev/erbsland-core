// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../String.hpp"
#include "../TextNodeData.hpp"

#include <utility>

namespace erbsland::text::impl {

/// URL metadata attached to a link node.
class LinkData final : public TextNodeData {
public:
    /// Create link metadata with its target `url`.
    explicit LinkData(String url) noexcept : _url{std::move(url)} {}

public:
    /// Get the link target URL.
    [[nodiscard]] auto url() const noexcept -> String { return _url; }
    [[nodiscard]] auto toString() const -> String override { return _url; }

private:
    String _url; ///< The link target URL.
};

}
