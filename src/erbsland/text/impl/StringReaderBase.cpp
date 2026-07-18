// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringReaderBase.hpp"

namespace erbsland::text::impl {

using util::LoopResult;
using util::LoopStatus;

auto StringReaderBase::readWhile(const ReadFn &readFn, const CharSet &expected, unit::CpLength maximum) noexcept
    -> LoopResult {
    auto count = unit::CpLength::zero();
    while (true) {
        const auto state = save();
        const auto character = read();
        if (character.isEndOfData()) {
            return LoopResult::EndOfData;
        }
        if (!expected.contains(character)) {
            restore(state);
            return LoopResult::Success;
        }
        if (!maximum.isInfinite() && count >= maximum) {
            restore(state);
            return LoopResult::LimitReached;
        }
        const auto readFnResult = readFn(character);
        if (readFnResult != LoopStatus::Continue) {
            restore(state);
            return readFnResult == LoopStatus::Error ? LoopResult::Error : LoopResult::Stopped;
        }
        ++count;
    }
}

auto StringReaderBase::readUntil(const ReadFn &readFn, const CharSet &stopSet, unit::CpLength maximum) noexcept
    -> LoopResult {
    auto count = unit::CpLength::zero();
    while (true) {
        const auto state = save();
        const auto character = read();
        if (character.isEndOfData()) {
            return LoopResult::EndOfData;
        }
        if (stopSet.contains(character)) {
            restore(state);
            return LoopResult::Success;
        }
        if (!maximum.isInfinite() && count >= maximum) {
            restore(state);
            return LoopResult::LimitReached;
        }
        const auto readFnResult = readFn(character);
        if (readFnResult != LoopStatus::Continue) {
            restore(state);
            return readFnResult == LoopStatus::Error ? LoopResult::Error : LoopResult::Stopped;
        }
        ++count;
    }
}

}
