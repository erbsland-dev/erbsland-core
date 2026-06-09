// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionDisplayText.hpp"
#include "OptionRenderer.hpp"

#include <utility>

namespace erbsland::options {

/// Base class for option renderers that use configurable display text.
/// @tested{StandardOptionRendererTest TerminalOptionsRendererTest}
class OptionRendererBase : public OptionRenderer {
public:
    /// Create a renderer base with the default display text.
    OptionRendererBase();
    /// Create a renderer base with explicit display text.
    explicit OptionRendererBase(OptionDisplayText displayText);

    // defaults
    ~OptionRendererBase() override = default;
    OptionRendererBase(const OptionRendererBase &) = default;
    auto operator=(const OptionRendererBase &) -> OptionRendererBase & = default;
    OptionRendererBase(OptionRendererBase &&) = default;
    auto operator=(OptionRendererBase &&) -> OptionRendererBase & = default;

public: // accessors
    /// Get the active display text.
    [[nodiscard]] auto displayText() const noexcept -> const OptionDisplayText & { return _displayText; }
    /// Set the active display text.
    void setDisplayText(OptionDisplayText displayText) { _displayText = std::move(displayText); }

private:
    OptionDisplayText _displayText; ///< The active display text.
};

}
