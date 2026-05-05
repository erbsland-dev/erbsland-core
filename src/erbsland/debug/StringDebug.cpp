// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringDebug.hpp"

#include "impl/StringDebugBuilder.hpp"

namespace erbsland::debug {

using namespace text::literals;

auto toDebugTree(const text::U8String &value, const DebugViewDetails details) -> text::StringTree {
    return impl::makeStringDebugTree("U8String"_el, value, details);
}

auto toDebugTree(const text::U8StringView &value, const DebugViewDetails details) -> text::StringTree {
    return impl::makeStringDebugTree("U8StringView"_el, value, details);
}

auto toDebugTree(const text::U16String &value, const DebugViewDetails details) -> text::StringTree {
    return impl::makeStringDebugTree("U16String"_el, value, details);
}

auto toDebugTree(const text::U16StringView &value, const DebugViewDetails details) -> text::StringTree {
    return impl::makeStringDebugTree("U16StringView"_el, value, details);
}

auto toDebugTree(const text::U32String &value, const DebugViewDetails details) -> text::StringTree {
    return impl::makeStringDebugTree("U32String"_el, value, details);
}

auto toDebugTree(const text::U32StringView &value, const DebugViewDetails details) -> text::StringTree {
    return impl::makeStringDebugTree("U32StringView"_el, value, details);
}

}
