// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegEx.hpp"

#include "impl/text/Replacement.hpp"

#include "../unit/ByteRange.hpp"

namespace erbsland::re {

auto RegEx::replaceAll(const text::StringView &subject, const text::StringView &replacementExpression) const
    -> text::String {
    if (subject.isEmpty() && replacementExpression.isEmpty()) {
        return {};
    }
    const auto replacement = impl::Replacement::create(replacementExpression, _engine->captureGroupNames());
    text::String result;
    auto lastEnd = unit::ByteIndex::zero();
    for (const auto &match : findAll(subject)) {
        const auto matchBegin = unit::ByteIndex::fromSizeT(match->begin());
        const auto matchEnd = unit::ByteIndex::fromSizeT(match->end());
        if (matchBegin > lastEnd) {
            result.append(subject.slice(unit::ByteRange{lastEnd, matchBegin}));
        }
        replacement.appendTo(result, match);
        lastEnd = matchEnd;
    }
    const auto textEnd = unit::ByteIndex::end(subject.length());
    if (lastEnd < textEnd) {
        result.append(subject.slice(unit::ByteRange{lastEnd, textEnd}));
    }
    return result;
}

auto RegEx::replaceAll(const text::StringView &subject, const ReplaceFn &replaceFn) const -> text::String {
    text::String result;
    auto lastEnd = unit::ByteIndex::zero();
    for (const auto &match : findAll(subject)) {
        const auto matchBegin = unit::ByteIndex::fromSizeT(match->begin());
        const auto matchEnd = unit::ByteIndex::fromSizeT(match->end());
        if (matchBegin > lastEnd) {
            result.append(subject.slice(unit::ByteRange{lastEnd, matchBegin}));
        }
        result.append(replaceFn(match));
        lastEnd = matchEnd;
    }
    const auto textEnd = unit::ByteIndex::end(subject.length());
    if (lastEnd < textEnd) {
        result.append(subject.slice(unit::ByteRange{lastEnd, textEnd}));
    }
    return result;
}

}
