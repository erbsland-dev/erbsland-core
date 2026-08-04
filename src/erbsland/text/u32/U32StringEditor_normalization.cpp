// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringEditor.hpp"

#include "../impl/StringNormalizationTools.hpp"

#include <utility>

namespace erbsland::text {

auto U32StringEditor::normalize(const NormalizationForm form) -> U32StringEditor & {
    if (auto storage = impl::StringNormalizationTools{*this, form}.normalizedIfChanged()) {
        *this = U32StringEditor{std::move(*storage)};
    }
    return *this;
}

auto U32StringEditor::normalized(const NormalizationForm form) const -> U32StringEditor {
    if (auto storage = impl::StringNormalizationTools{*this, form}.normalizedIfChanged()) {
        return U32StringEditor{std::move(*storage)};
    }
    return *this;
}

}
