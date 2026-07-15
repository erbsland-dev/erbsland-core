// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/StringView.hpp"

#include <memory>

namespace erbsland::i18n {

/// Translates one registered English display text.
///
/// Return an empty string to keep the English source text.
/// @tested{DisplayTextMapTest}
class DisplayTextTranslator {
protected:
    DisplayTextTranslator() = default;

public:
    virtual ~DisplayTextTranslator() = default;
    DisplayTextTranslator(const DisplayTextTranslator &) = delete;
    DisplayTextTranslator(DisplayTextTranslator &&) = delete;
    auto operator=(const DisplayTextTranslator &) -> DisplayTextTranslator & = delete;
    auto operator=(DisplayTextTranslator &&) -> DisplayTextTranslator & = delete;

public:
    /// Translate one display text.
    /// @param key The originally requested display-text key.
    /// @param sourceText The resolved English source text.
    /// @return The translated text, or an empty string to keep `sourceText`.
    [[nodiscard]] virtual auto translate(const text::StringView &key, const text::StringView &sourceText) const
        -> text::StringView = 0;
};

using DisplayTextTranslatorPtr = std::shared_ptr<DisplayTextTranslator>;
using DisplayTextTranslatorConstPtr = std::shared_ptr<const DisplayTextTranslator>;

}
