// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringReaderBase.hpp"

#include "../../err/LogicFailure.hpp"

namespace erbsland::text::impl {

using namespace unit;
using util::LoopResult;
using util::LoopStatus;

auto StringReaderBase::u8Position() const -> ByteIndex {
    throw err::LogicFailure{"u8Position requires a UTF-8 string reader."};
}

auto StringReaderBase::u16Position() const -> U16Index {
    throw err::LogicFailure{"u16Position requires a UTF-16 string reader."};
}

auto StringReaderBase::u32Position() const -> CpIndex {
    throw err::LogicFailure{"u32Position requires a UTF-32 string reader."};
}

auto StringReaderBase::readWhile(const ReadFn &readFn, const CharSet &expected, CpCount maximum) noexcept
    -> LoopResult {
    auto count = CpCount::zero();
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

auto StringReaderBase::readUntil(const ReadFn &readFn, const CharSet &stopSet, CpCount maximum) noexcept -> LoopResult {
    auto count = CpCount::zero();
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
