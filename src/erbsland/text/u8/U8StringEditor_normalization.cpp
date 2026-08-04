// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringEditor.hpp"

#include "../impl/StringNormalizationTools.hpp"

#include <utility>

namespace erbsland::text {

auto U8StringEditor::normalize(const NormalizationForm form) -> U8StringEditor & {
    if (auto storage = impl::StringNormalizationTools{*this, form}.normalizedIfChanged()) {
        *this = U8StringEditor{std::move(*storage)};
    }
    return *this;
}

auto U8StringEditor::normalized(const NormalizationForm form) const -> U8StringEditor {
    if (auto storage = impl::StringNormalizationTools{*this, form}.normalizedIfChanged()) {
        return U8StringEditor{std::move(*storage)};
    }
    return *this;
}

}
