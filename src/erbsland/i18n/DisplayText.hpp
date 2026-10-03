// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DisplayText_fwd.hpp"
#include "DisplayTextData_fwd.hpp"
#include "DisplayTextTranslator.hpp"

#include "../text/String.hpp"
#include "../text/StringFormat.hpp"

namespace erbsland::i18n {

/// The application-owned, thread-safe registry of display-text layers and cached translations.
/// @seedoc{/reference/i18n/internationalization}
/// @tested{DisplayTextTest}
class DisplayText {
protected:
    // defaults
    DisplayText() = default;

public:
    // defaults/deletions
    virtual ~DisplayText() = default;
    DisplayText(const DisplayText &) = delete;
    DisplayText(DisplayText &&) = delete;
    auto operator=(const DisplayText &) -> DisplayText & = delete;
    auto operator=(DisplayText &&) -> DisplayText & = delete;

public: // access
    /// Resolve a normalized key, applying optional translation and caching the result.
    /// @param key The requested key; missing entries return this unchanged.
    /// @return The display text.
    [[nodiscard]] virtual auto text(const text::String &key) const -> text::String = 0;
    /// Resolve, translate, and cache a reusable format.
    /// @param key The requested normalized key.
    /// @return The compiled format.
    /// @throws text::FormatError if the resolved text is not a valid format.
    [[nodiscard]] virtual auto format(const text::String &key) const -> text::StringFormat = 0;
    /// Access the currently installed translator, or null.
    [[nodiscard]] virtual auto translator() const -> DisplayTextTranslatorConstPtr = 0;

public: // registration
    /// Add immutable data as the newest layer; adding an existing instance is a no-op.
    /// @param data Non-null data whose arrays remain immutable while registered.
    virtual void addData(DisplayTextDataConstPtr data) = 0;
    /// Remove registered data, preserving the built-in bottom layer.
    /// @param data A non-null data instance.
    /// @return Whether an application layer was removed.
    [[nodiscard]] virtual auto removeData(const DisplayTextDataConstPtr &data) -> bool = 0;
    /// Replace the translator and invalidate all caches, even if the pointer is unchanged.
    /// Translator callbacks must not reenter this registry.
    /// @param translator The translator, or null to restore untranslated source text.
    virtual void setTranslator(DisplayTextTranslatorConstPtr translator) = 0;
};

/// Resolve a display text through the application registry.
/// @param key The normalized display-text key.
/// @return The display text, or the requested key when missing.
/// @tested{DisplayTextTest}
[[nodiscard]] auto displayText(const text::String &key) -> text::String;

/// Resolve a reusable format through the application registry.
/// @param key The normalized display-text key.
/// @return The compiled display format.
/// @throws text::FormatError if the resolved text is not a valid format.
/// @tested{DisplayTextTest}
[[nodiscard]] auto displayFormat(const text::String &key) -> text::StringFormat;

}
