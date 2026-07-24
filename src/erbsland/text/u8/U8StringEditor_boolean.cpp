// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringEditor.hpp"

#include "U8String.hpp"

#include "../impl/BooleanConversion.hpp"

namespace erbsland::text {

auto U8StringEditor::toBoolean(const bool defaultValue) const noexcept -> bool {
    return impl::parseBooleanOrDefault(*this, impl::cU8BooleanLiterals, defaultValue);
}

auto U8StringEditor::toBooleanOrThrow() const -> bool {
    return impl::parseBooleanOrThrow(*this, impl::cU8BooleanLiterals);
}

}
