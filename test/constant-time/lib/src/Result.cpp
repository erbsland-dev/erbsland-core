// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Result.hpp"

#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>

#include <cmath>

namespace app::constant_time {

using namespace el::text::literals;

auto Result::outcomeName() const -> el::String {
    switch (_outcome) {
    case Outcome::NoLeakage:
        return "no-leakage-detected-within-budget"_el;
    case Outcome::Leakage:
        return "leakage-detected"_el;
    case Outcome::Insufficient:
        return "insufficient-evidence"_el;
    case Outcome::Unavailable:
        return "unavailable"_el;
    case Outcome::Error:
        return "error"_el;
    case Outcome::Interrupted:
        return "interrupted"_el;
    }
    return "error"_el;
}

auto Result::toString() const -> el::String {
    return el::StringFormat{
        "record=result test={} outcome={} backend={} populations={} seed={} repetitions={} samples-0={} samples-1={} rejected={} elapsed={} max-t={} channel={} analysis-samples-0={} analysis-samples-1={} detail={}"_el}
        .build(
            _metadata.id,
            outcomeName(),
            _metadata.backend,
            _metadata.populations,
            _seed,
            _repetitions,
            _samples[0],
            _samples[1],
            _rejected,
            _elapsed.toString(),
            std::abs(_evidence.statistic),
            _evidence.channel,
            _evidence.samples[0],
            _evidence.samples[1],
            _detail);
}

}
