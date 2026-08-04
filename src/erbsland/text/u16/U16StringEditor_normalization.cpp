// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringEditor.hpp"

#include "../impl/StringNormalizationTools.hpp"

#include <utility>

namespace erbsland::text {

auto U16StringEditor::normalize(const NormalizationForm form) -> U16StringEditor & {
    if (auto storage = impl::StringNormalizationTools{*this, form}.normalizedIfChanged()) {
        *this = U16StringEditor{std::move(*storage)};
    }
    return *this;
}

auto U16StringEditor::normalized(const NormalizationForm form) const -> U16StringEditor {
    if (auto storage = impl::StringNormalizationTools{*this, form}.normalizedIfChanged()) {
        return U16StringEditor{std::move(*storage)};
    }
    return *this;
}

}
