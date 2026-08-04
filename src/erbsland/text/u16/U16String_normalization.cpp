// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16String.hpp"

#include "U16StringEditor.hpp"

#include "../impl/StringNormalizationTools.hpp"

#include <utility>

namespace erbsland::text {

auto U16String::normalized(const NormalizationForm form) const -> U16String {
    if (auto storage = impl::StringNormalizationTools{*this, form}.normalizedIfChanged()) {
        return U16String{U16StringEditor{std::move(*storage)}};
    }
    return *this;
}

}
