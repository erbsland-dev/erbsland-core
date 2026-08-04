// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32String.hpp"

#include "U32StringEditor.hpp"

#include "../impl/StringNormalizationTools.hpp"

#include <utility>

namespace erbsland::text {

auto U32String::normalized(const NormalizationForm form) const -> U32String {
    if (auto storage = impl::StringNormalizationTools{*this, form}.normalizedIfChanged()) {
        return U32String{U32StringEditor{std::move(*storage)}};
    }
    return *this;
}

}
