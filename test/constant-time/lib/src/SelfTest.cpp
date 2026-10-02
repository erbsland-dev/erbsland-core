// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SelfTest.hpp"

#include "Registry.hpp"
#include "Statistics.hpp"

#include "cases/control/LeakyCase.hpp"

#include <erbsland/err/LogicError.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/unit/ItemCount.hpp>

#include <cmath>

namespace app::constant_time {

using namespace el::text::literals;

void SelfTest::checkDeterministic() const {
    auto moments = Moments{};
    for (const auto value : {1.0, 2.0, 3.0}) {
        moments.add(value);
    }
    require(moments.mean() == 2.0 && moments.variance() == 1.0);
    auto test = WelchTest{};
    for (auto index = 0; index < 10000; ++index) {
        test.add(100.0 + static_cast<double>(index % 2), false);
        test.add(200.0 + static_cast<double>(index % 2), true);
    }
    require(test.isEligible() && std::abs(test.statistic()) > 10);
    auto registry = Registry{};
    registry.add({"check"_el, {}});
    require(registry.select(el::StringList{"check"_el, "check"_el}, false).count() == el::ItemCount{1});
}

auto SelfTest::runControl(Runner &runner) const -> Result {
    auto test = LeakyCase{};
    auto result = runner.run(test, {.duration = el::Seconds{10}, .seed = 1234567});
    if (result.outcome() == Outcome::Leakage) {
        result.setDetail("Deliberately leaky control detected; reproduce with --self-test."_el);
    }
    return result;
}

void SelfTest::require(const bool condition) {
    if (!condition) {
        throw el::LogicError{"Deterministic harness check failed."_el};
    }
}

}
