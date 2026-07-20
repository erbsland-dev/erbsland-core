// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringDebug.hpp"

#include "impl/StringDebugBuilder.hpp"

namespace erbsland::debug {

using namespace text::literals;
using namespace text;

auto toDebugTree(const U8StringEditor &value, const DebugViewDetails details) -> StringTree {
    return impl::makeStringDebugTree("U8StringEditor"_el, value, details);
}

auto toDebugTree(const U8String &value, const DebugViewDetails details) -> StringTree {
    return impl::makeStringDebugTree("U8String"_el, value, details);
}

auto toDebugTree(const U16StringEditor &value, const DebugViewDetails details) -> StringTree {
    return impl::makeStringDebugTree("U16StringEditor"_el, value, details);
}

auto toDebugTree(const U16String &value, const DebugViewDetails details) -> StringTree {
    return impl::makeStringDebugTree("U16String"_el, value, details);
}

auto toDebugTree(const U32StringEditor &value, const DebugViewDetails details) -> StringTree {
    return impl::makeStringDebugTree("U32StringEditor"_el, value, details);
}

auto toDebugTree(const U32String &value, const DebugViewDetails details) -> StringTree {
    return impl::makeStringDebugTree("U32String"_el, value, details);
}

}
