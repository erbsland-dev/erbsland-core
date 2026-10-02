// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Statistics.hpp"

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unit/ItemIndex.hpp>
#include <erbsland/util/List.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace app::constant_time {

using namespace el::text::literals;

void Statistics::initialize(el::List<double> pilot) {
    if (pilot.isEmpty()) {
        throw el::ParameterError{"Empty timing pilot."_el, "pilot"_el};
    }
    pilot.sort();
    for (auto index = std::size_t{}; index < _thresholds.size(); ++index) {
        const auto percentile = 1.0 - std::pow(0.5, 10.0 * static_cast<double>(index + 1) / 100.0);
        const auto position = std::min(
            static_cast<std::size_t>(static_cast<double>(pilot.count().toSizeT()) * percentile),
            pilot.count().toSizeT() - 1);
        _thresholds[index] = pilot.getRefOrThrow(el::ItemIndex{position});
    }
}

void Statistics::add(const double nanoseconds, const bool population) {
    if (!std::isfinite(nanoseconds) || nanoseconds <= 0) {
        return;
    }
    _tests[0].add(nanoseconds, population);
    for (auto index = std::size_t{}; index < _thresholds.size(); ++index) {
        if (nanoseconds < _thresholds[index]) {
            _tests[index + 1].add(nanoseconds, population);
        }
    }
    if (_tests[0].population(false).count() >= 10000 && _tests[0].population(true).count() >= 10000) {
        const auto centered = nanoseconds - _tests[0].population(population).mean();
        _tests[101].add(centered * centered, population);
    }
}

auto Statistics::evidence() const -> Evidence {
    auto result = Evidence{};
    for (auto index = std::size_t{}; index < _tests.size(); ++index) {
        if (!_tests[index].isEligible()) {
            continue;
        }
        const auto statistic = _tests[index].statistic();
        if (!result.eligible || std::abs(statistic) > std::abs(result.statistic)) {
            result = {
                statistic,
                index == 0         ? el::String{"uncropped"_el}
                    : index == 101 ? el::String{"second-order"_el}
                                   : el::StringFormat{"cropped-{}"_el}.build(index),
                {_tests[index].population(false).count(), _tests[index].population(true).count()},
                true};
        }
    }
    return result;
}

auto Statistics::hasEnoughEvidence() const noexcept -> bool {
    return _tests[0].isEligible() && _tests[101].isEligible();
}
}
