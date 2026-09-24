// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LzmaProbabilities.hpp"

#include <algorithm>

namespace erbsland::compression::impl {

LzmaProbabilities::LzmaProbabilities() {
    std::ranges::fill(isMatch, LzmaLengthProbabilities::cInitialProbability);
    std::ranges::fill(isRep, LzmaLengthProbabilities::cInitialProbability);
    std::ranges::fill(isRepG0, LzmaLengthProbabilities::cInitialProbability);
    std::ranges::fill(isRepG1, LzmaLengthProbabilities::cInitialProbability);
    std::ranges::fill(isRepG2, LzmaLengthProbabilities::cInitialProbability);
    std::ranges::fill(isRep0Long, LzmaLengthProbabilities::cInitialProbability);
    std::ranges::fill(literals, LzmaLengthProbabilities::cInitialProbability);
    std::ranges::fill(posSlot, LzmaLengthProbabilities::cInitialProbability);
    std::ranges::fill(posDecoders, LzmaLengthProbabilities::cInitialProbability);
    std::ranges::fill(posAlign, LzmaLengthProbabilities::cInitialProbability);
    initializeLzmaLengthProbabilities(matchLzmaLengthProbabilities);
    initializeLzmaLengthProbabilities(repLzmaLengthProbabilities);
}

void LzmaProbabilities::initializeLzmaLengthProbabilities(LzmaLengthProbabilities &length) {
    std::ranges::fill(length.low, LzmaLengthProbabilities::cInitialProbability);
    std::ranges::fill(length.mid, LzmaLengthProbabilities::cInitialProbability);
    std::ranges::fill(length.high, LzmaLengthProbabilities::cInitialProbability);
}

}
