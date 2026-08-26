// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../String.hpp"

namespace erbsland::text::render {

/// One loaded layout source and its cache identity.
/// @seedoc{/reference/text/render}
/// @tested{RenderEnvironmentTest FileSystemLoaderTest}
class LayoutSource final {
public:
    /// Create a loaded source.
    /// @param text The UTF-8 layout text.
    /// @param origin A stable diagnostic origin, such as an absolute path.
    /// @param revision An opaque token that changes whenever the source changes.
    LayoutSource(String text, String origin, String revision) noexcept :
        _text{std::move(text)}, _origin{std::move(origin)}, _revision{std::move(revision)} {}

    // defaults
    LayoutSource(const LayoutSource &) = default;
    LayoutSource(LayoutSource &&) = default;
    auto operator=(const LayoutSource &) -> LayoutSource & = default;
    auto operator=(LayoutSource &&) -> LayoutSource & = default;

public: // accessors
    /// Access the UTF-8 source text.
    [[nodiscard]] auto text() const noexcept -> const String & { return _text; }
    /// Access the diagnostic source origin.
    [[nodiscard]] auto origin() const noexcept -> const String & { return _origin; }
    /// Access the opaque source revision.
    [[nodiscard]] auto revision() const noexcept -> const String & { return _revision; }

private:
    String _text;     ///< The complete UTF-8 layout source.
    String _origin;   ///< The stable source origin used in diagnostics.
    String _revision; ///< The opaque cache revision.
};

}
