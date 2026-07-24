// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegEx.hpp"

#include "Match.hpp"

#include "impl/engine/Engine.hpp"
#include "impl/text/Replacement.hpp"

#include "../text/StringEditor.hpp"
#include "../unit/ByteRange.hpp"

namespace erbsland::re {

using unit::ByteIndex;
using unit::ByteRange;

auto RegEx::replaceAll(const text::String &subject, const text::String &replacementExpression) const -> text::String {
    if (subject.isEmpty() && replacementExpression.isEmpty()) {
        return {};
    }
    const auto &engine = this->engine();
    const auto replacement = impl::Replacement::create(replacementExpression, engine->captureGroupNames());
    text::StringEditor result;
    auto lastEnd = ByteIndex::zero();
    for (const auto &match : findAll(subject)) {
        const auto matchBegin = ByteIndex::fromSizeT(match->begin());
        const auto matchEnd = ByteIndex::fromSizeT(match->end());
        if (matchBegin > lastEnd) {
            result.append(subject.slice(ByteRange{lastEnd, matchBegin}));
        }
        replacement.appendTo(result, match);
        lastEnd = matchEnd;
    }
    const auto textEnd = ByteIndex::end(subject.length());
    if (lastEnd < textEnd) {
        result.append(subject.slice(ByteRange{lastEnd, textEnd}));
    }
    return result;
}

auto RegEx::replaceAll(const text::String &subject, const ReplaceFn &replaceFn) const -> text::String {
    text::StringEditor result;
    auto lastEnd = ByteIndex::zero();
    for (const auto &match : findAll(subject)) {
        const auto matchBegin = ByteIndex::fromSizeT(match->begin());
        const auto matchEnd = ByteIndex::fromSizeT(match->end());
        if (matchBegin > lastEnd) {
            result.append(subject.slice(ByteRange{lastEnd, matchBegin}));
        }
        result.append(replaceFn(match));
        lastEnd = matchEnd;
    }
    const auto textEnd = ByteIndex::end(subject.length());
    if (lastEnd < textEnd) {
        result.append(subject.slice(ByteRange{lastEnd, textEnd}));
    }
    return result;
}

}
