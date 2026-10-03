// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RenderFormat.hpp"

namespace erbsland::cterm {

/// Fluent options controlling buffer serialization.
/// @seedoc{/reference/cterm/buffer_rendering}
/// @tested{BufferRenderTest}
class BufferRenderOptions final {
public:
    // defaults
    BufferRenderOptions() = default;

public: // accessors
    /// Get the selected output representation.
    [[nodiscard]] auto format() const noexcept -> RenderFormat { return _format; }
    /// Select the output representation.
    auto setFormat(RenderFormat format) noexcept -> BufferRenderOptions & {
        _format = format;
        return *this;
    }
    /// Test whether default-style spaces at the end of a row are omitted.
    [[nodiscard]] auto trimTrailingSpaces() const noexcept -> bool { return _trimTrailingSpaces; }
    /// Select removal of default-style spaces at the end of rows.
    auto setTrimTrailingSpaces(bool enabled) noexcept -> BufferRenderOptions & {
        _trimTrailingSpaces = enabled;
        return *this;
    }
    /// Test whether default-style empty bottom rows are omitted.
    [[nodiscard]] auto trimTrailingEmptyLines() const noexcept -> bool { return _trimTrailingEmptyLines; }
    /// Select removal of default-style empty bottom rows.
    auto setTrimTrailingEmptyLines(bool enabled) noexcept -> BufferRenderOptions & {
        _trimTrailingEmptyLines = enabled;
        return *this;
    }

private:
    RenderFormat _format{RenderFormat::Ansi}; ///< Selected representation.
    bool _trimTrailingSpaces{false};          ///< Omit default-style row padding.
    bool _trimTrailingEmptyLines{false};      ///< Omit default-style bottom padding.
};

}
