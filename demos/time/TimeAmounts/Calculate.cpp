// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeAmountsDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Compare like units and calculate signed totals with integer scaling and division.
/// @notest{Compiled and executed documentation demo.}
void calculate() {
    const auto sample = el::Milliseconds{250};
    const auto pause = el::Seconds{1}.converted<el::Milliseconds>();
    const auto budget = el::Milliseconds{2000};
    auto required = sample * 3 + pause;
    required += el::Milliseconds{100};
    required -= el::Milliseconds{50};
    el::io::printLine(
        el::StringFormat{"Required: {} ms; remaining: {} ms; within budget: {}; equal budget: {}"_el}.build(
            required.toRawValue(), (budget - required).toRawValue(), required <= budget, required == budget));

    const auto correction = -sample;
    const auto share = el::Milliseconds{1001} / 4;
    const auto negativeShare = el::Milliseconds{-1001} / 4;
    el::io::printLine(
        el::StringFormat{"Correction: {} ms; share: {} ms; negative share: {} ms; cancelled: {}"_el}.build(
            correction.toRawValue(), share.toRawValue(), negativeShare.toRawValue(), (sample + correction).isZero()));
    auto scaled = sample;
    scaled *= 4;
    scaled /= 2;
    el::io::printLine(
        el::StringFormat{"Scaled in place: {} ms; same as two samples: {}"_el}.build(
            scaled.toRawValue(), scaled == sample + sample));
}

}
