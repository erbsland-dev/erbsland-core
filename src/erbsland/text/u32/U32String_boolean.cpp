// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32String.hpp"

#include "../impl/BooleanConversion.hpp"

namespace erbsland::text {

auto U32String::toBoolean(const bool defaultValue) const noexcept -> bool {
    return impl::parseBooleanOrDefault(*this, impl::cU32BooleanLiterals, defaultValue);
}

auto U32String::toBooleanOrThrow() const -> bool {
    return impl::parseBooleanOrThrow(*this, impl::cU32BooleanLiterals);
}

}
