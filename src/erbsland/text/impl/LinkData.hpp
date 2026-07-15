// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../StringView.hpp"
#include "../TextNodeData.hpp"

#include <utility>

namespace erbsland::text::impl {

/// URL metadata attached to a link node.
class LinkData final : public TextNodeData {
public:
    explicit LinkData(StringView url) noexcept : _url{std::move(url)} {}

public:
    [[nodiscard]] auto url() const noexcept -> StringView { return _url; }
    [[nodiscard]] auto toString() const -> StringView override { return _url; }

private:
    StringView _url; ///< The link target URL.
};

}
