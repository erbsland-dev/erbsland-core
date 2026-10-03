// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegExCached.hpp"

#include "Match.hpp"
#include "Match16.hpp"
#include "Match32.hpp"
#include "RegEx.hpp"

#include "impl/CachedMatches.hpp"

#include "../core/impl/application_data/ApplicationRegExData.hpp"
#include "../core/impl/ApplicationDataAccess.hpp"
#include "../text/String.hpp"
#include "../text/u16/U16String.hpp"
#include "../text/u32/U32String.hpp"

namespace erbsland::re {

using namespace text;

auto compile(StringLiteral pattern) -> ConstRegExPtr {
    return core::impl::applicationData()->regularExpressions().get()->cache().compile(pattern);
}

auto match(StringLiteral pattern, const String &text) -> MatchPtr {
    return compile(pattern)->match(text);
}

auto match(StringLiteral pattern, const U16String &text) -> Match16Ptr {
    return compile(pattern)->match(text);
}

auto match(StringLiteral pattern, const U32String &text) -> Match32Ptr {
    return compile(pattern)->match(text);
}

auto fullMatch(StringLiteral pattern, const String &text) -> MatchPtr {
    return compile(pattern)->fullMatch(text);
}

auto fullMatch(StringLiteral pattern, const U16String &text) -> Match16Ptr {
    return compile(pattern)->fullMatch(text);
}

auto fullMatch(StringLiteral pattern, const U32String &text) -> Match32Ptr {
    return compile(pattern)->fullMatch(text);
}

auto findFirst(StringLiteral pattern, const String &text) -> MatchPtr {
    return compile(pattern)->findFirst(text);
}

auto findFirst(StringLiteral pattern, const U16String &text) -> Match16Ptr {
    return compile(pattern)->findFirst(text);
}

auto findFirst(StringLiteral pattern, const U32String &text) -> Match32Ptr {
    return compile(pattern)->findFirst(text);
}

auto findAll(StringLiteral pattern, const String &text) -> MatchGenerator {
    return impl::cachedMatches(compile(pattern), text);
}

auto findAll(StringLiteral pattern, const U16String &text) -> Match16Generator {
    return impl::cachedMatches(compile(pattern), text);
}

auto findAll(StringLiteral pattern, const U32String &text) -> Match32Generator {
    return impl::cachedMatches(compile(pattern), text);
}

auto collectAll(StringLiteral pattern, const String &text) -> MatchList {
    return compile(pattern)->collectAll(text);
}

auto collectAll(StringLiteral pattern, const U16String &text) -> Match16List {
    return compile(pattern)->collectAll(text);
}

auto collectAll(StringLiteral pattern, const U32String &text) -> Match32List {
    return compile(pattern)->collectAll(text);
}

auto replaceAll(StringLiteral pattern, const String &text, StringLiteral replacement) -> String {
    return compile(pattern)->replaceAll(text, replacement);
}

}
