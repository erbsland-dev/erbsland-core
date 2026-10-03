// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../DisplayText.hpp"

#include "../../core/impl/application_data/ApplicationSystemData_fwd.hpp"
#include "../../text/StringHashMap.hpp"

#include <mutex>
#include <optional>
#include <vector>

namespace erbsland::i18n::impl {

/// Implements the application registry with specificity-first layer lookup.
/// @tested{DisplayTextTest}
class DisplayText final : public i18n::DisplayText {
    friend class core::impl::ApplicationSystemData;

private:
    /// Create a registry with the permanent Core bottom layer.
    DisplayText();

public: // implement DisplayText
    [[nodiscard]] auto text(const text::String &key) const -> text::String override;
    [[nodiscard]] auto format(const text::String &key) const -> text::StringFormat override;
    [[nodiscard]] auto translator() const -> DisplayTextTranslatorConstPtr override;
    void addData(DisplayTextDataConstPtr data) override;
    [[nodiscard]] auto removeData(const DisplayTextDataConstPtr &data) -> bool override;
    void setTranslator(DisplayTextTranslatorConstPtr translator) override;

private:
    /// Resolve and cache text while holding the registry mutex.
    [[nodiscard]] auto resolveText(const text::String &key) const -> text::String;
    /// Find the source across layers and fallback candidates.
    [[nodiscard]] auto findSourceText(const text::String &key) const -> std::optional<text::String>;
    /// Find an exact candidate across all layers, newest first.
    [[nodiscard]] auto findInLayers(const text::String &key) const -> std::optional<text::String>;
    /// Clear caches while holding the mutex.
    void clearCaches();
    /// Extract the final name and first-domain fallback.
    [[nodiscard]] static auto finalKey(const text::String &key) -> text::String;
    /// Extract the first-domain and final-name fallback.
    [[nodiscard]] static auto domainKey(const text::String &key) -> text::String;

private:
    mutable std::mutex _mutex;                            ///< Serializes registry access and translator callbacks.
    std::vector<DisplayTextDataConstPtr> _layers;         ///< Immutable source data, oldest first.
    DisplayTextTranslatorConstPtr _translator;            ///< Optional translation behavior.
    mutable text::StringHashMap<text::String> _textCache; ///< Resolved display texts by requested key.
    mutable text::StringHashMap<std::shared_ptr<const text::StringFormat>> _formatCache; ///< Compiled formats.
};

}
