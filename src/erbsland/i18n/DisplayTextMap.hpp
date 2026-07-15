// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DisplayTextMap_fwd.hpp"
#include "DisplayTextTranslator.hpp"

#include "../text/StringFormat.hpp"
#include "../text/StringHashMap.hpp"
#include "../text/StringView.hpp"

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
    [[nodiscard]] auto text(const text::StringView &key) const -> text::StringView;
    /// Resolve, translate, and compile a reusable format.
    [[nodiscard]] auto format(const text::StringView &key) const -> text::StringFormat;

public: // modifiers
    /// Set an English source text and clear all caches.
    auto set(text::StringView key, text::StringView sourceText) -> DisplayTextMap &;
    /// Remove a source text and clear all caches.
    auto remove(const text::StringView &key) -> DisplayTextMap &;
    /// Set the translator and clear all caches.
    auto setTranslator(DisplayTextTranslatorConstPtr translator) -> DisplayTextMap &;

public: // accessors
    /// Access the translator, if one is installed.
    [[nodiscard]] auto translator() const noexcept -> const DisplayTextTranslatorConstPtr & { return _translator; }

private:
    void addDefaultTexts();
    void clearCaches();
    [[nodiscard]] auto findSourceText(const text::StringView &key) const -> std::optional<text::StringView>;
    [[nodiscard]] static auto domainKey(const text::StringView &key) -> text::StringView;
    [[nodiscard]] static auto finalKey(const text::StringView &key) noexcept -> text::StringView;

private:
    text::StringHashMap<text::StringView> _sourceTexts;
    DisplayTextTranslatorConstPtr _translator;
    mutable std::mutex _cacheMutex;
    mutable text::StringHashMap<text::StringView> _textCache;
    mutable text::StringHashMap<std::shared_ptr<const text::StringFormat>> _formatCache;
};

}
