// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringEditor.hpp"

#include "U16String.hpp"

#include "../impl/BooleanConversion.hpp"

namespace erbsland::text {

auto U16StringEditor::toBoolean(const bool defaultValue) const noexcept -> bool {
    return impl::parseBooleanOrDefault(*this, impl::cU16BooleanLiterals, defaultValue);
}

auto U16StringEditor::toBooleanOrThrow() const -> bool {
    return impl::parseBooleanOrThrow(*this, impl::cU16BooleanLiterals);
}

}
