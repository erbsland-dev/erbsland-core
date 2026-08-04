// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DisplayTextMap_fwd.hpp"
#include "DisplayTextTranslator.hpp"

#include "../text/String.hpp"
#include "../text/StringFormat.hpp"
#include "../text/StringHashMap.hpp"

#include <memory>
#include <mutex>
#include <optional>

namespace erbsland::i18n {

/// Central map for English system display texts and translated results.
///
/// Publish edited maps as `DisplayTextMapConstPtr`. Concurrent const access is supported; concurrent mutation is not.
/// @tested{DisplayTextMapTest}
class DisplayTextMap final {
public:
    /// Create a map populated with the built-in English display texts.
    DisplayTextMap();

    // defaults/deletions
    ~DisplayTextMap() = default;
    DisplayTextMap(const DisplayTextMap &) = delete;
    DisplayTextMap(DisplayTextMap &&) = delete;
    auto operator=(const DisplayTextMap &) -> DisplayTextMap & = delete;
    auto operator=(DisplayTextMap &&) -> DisplayTextMap & = delete;

public:
    /// Access the immutable process-wide English default map.
    [[nodiscard]] static auto defaultMap() -> const DisplayTextMapConstPtr &;
    /// Clone source entries and the translator without copying caches.
    [[nodiscard]] auto clone() const -> DisplayTextMapPtr;

public: // access
    /// Resolve and translate a display text.
    /// Lookup order is the exact key, `domain.key`, and `key`. A missing entry returns the requested key.
    [[nodiscard]] auto text(const text::String &key) const -> text::String;
    /// Resolve, translate, and compile a reusable format.
    [[nodiscard]] auto format(const text::String &key) const -> text::StringFormat;

public: // modifiers
    /// Set an English source text and clear all caches.
    auto set(const text::String &key, text::String sourceText) -> DisplayTextMap &;
    /// Remove a source text and clear all caches.
    auto remove(const text::String &key) -> DisplayTextMap &;
    /// Set the translator and clear all caches.
    auto setTranslator(DisplayTextTranslatorConstPtr translator) -> DisplayTextMap &;

public: // accessors
    /// Access the translator, if one is installed.
    [[nodiscard]] auto translator() const noexcept -> const DisplayTextTranslatorConstPtr & { return _translator; }

private:
    /// Add built-in English source texts.
    void addDefaultTexts();
    /// Clear translated text and format caches.
    void clearCaches();
    /// Find source text for a complete display-text key.
    [[nodiscard]] auto findSourceText(const text::String &key) const -> std::optional<text::String>;
    /// Extract the domain part of a display-text key.
    [[nodiscard]] static auto domainKey(const text::String &key) -> text::String;
    /// Extract the final identifier part of a display-text key.
    [[nodiscard]] static auto finalKey(const text::String &key) noexcept -> text::String;

private:
    text::StringHashMap<text::String> _sourceTexts;
    DisplayTextTranslatorConstPtr _translator;
    mutable std::mutex _cacheMutex;
    mutable text::StringHashMap<text::String> _textCache;
    mutable text::StringHashMap<std::shared_ptr<const text::StringFormat>> _formatCache;
};

}
