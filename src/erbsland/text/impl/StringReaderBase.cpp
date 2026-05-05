// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringReaderBase.hpp"

namespace erbsland::text::impl {

auto StringReaderBase::readWhile(const ReadFn &readFn, const CharSet &expected, unit::CpLength maximum) noexcept
    -> util::LoopResult {
    auto count = unit::CpLength::zero();
    while (true) {
        const auto state = save();
        const auto character = read();
        if (character.isEndOfData()) {
            return util::LoopResult::EndOfData;
        }
        if (!expected.contains(character)) {
            restore(state);
            return util::LoopResult::Success;
        }
        if (!maximum.isInfinite() && count >= maximum) {
            restore(state);
            return util::LoopResult::LimitReached;
        }
        const auto readFnResult = readFn(character);
        if (readFnResult != util::LoopStatus::Continue) {
            restore(state);
            return readFnResult == util::LoopStatus::Error ? util::LoopResult::Error : util::LoopResult::Stopped;
        }
        ++count;
    }
}

auto StringReaderBase::readUntil(const ReadFn &readFn, const CharSet &stopSet, unit::CpLength maximum) noexcept
    -> util::LoopResult {
    auto count = unit::CpLength::zero();
    while (true) {
        const auto state = save();
        const auto character = read();
        if (character.isEndOfData()) {
            return util::LoopResult::EndOfData;
        }
        if (stopSet.contains(character)) {
            restore(state);
            return util::LoopResult::Success;
        }
        if (!maximum.isInfinite() && count >= maximum) {
            restore(state);
            return util::LoopResult::LimitReached;
        }
        const auto readFnResult = readFn(character);
        if (readFnResult != util::LoopStatus::Continue) {
            restore(state);
            return readFnResult == util::LoopStatus::Error ? util::LoopResult::Error : util::LoopResult::Stopped;
        }
        ++count;
    }
}

}
