// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Runner.hpp"

#include "Checksum.hpp"
#include "Statistics.hpp"

#include <erbsland/err/Exception.hpp>
#include <erbsland/math/SaturatingMath.hpp>
#include <erbsland/random/FastRandom.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unit/ItemIndex.hpp>
#include <erbsland/util/List.hpp>

#include <algorithm>
#include <cmath>
#include <exception>

namespace app::constant_time {

using namespace el::text::literals;

auto Runner::run(TestCase &test, const RunOptions &options, std::function<void(const Result &)> progress) -> Result {
    const auto start = options.start.value_or(_clock());
    auto result = Result{test.metadata(), Outcome::Insufficient, options.seed};
    auto random = el::FastRandom{options.seed};
    auto statistics = Statistics{};
    const auto stopped = [&]() -> bool {
        return (_interrupted != nullptr && _interrupted->load(std::memory_order_relaxed)) ||
            _clock() - start >= options.duration;
    };
    const auto measure = [&](std::size_t index) -> double {
        const auto before = _clock();
        const auto checksum = test.execute(index, result._repetitions);
        const auto after = _clock();
        Checksum::instance().retain(checksum);
        return static_cast<double>((after - before).toNanoseconds().toRawValue());
    };
    try {
        if (!options.duration.isPositive() || test.fixtureBytes().isZero() ||
            test.fixtureBytes().toRawValue() > 64U * 1024U * 1024U) {
            result._outcome = Outcome::Error;
            result._detail = "Invalid duration or fixture memory estimate."_el;
        } else {
            auto batchSize = std::min<uint64_t>(4096, 64U * 1024U * 1024U / test.fixtureBytes().toRawValue());
            // Establish a timer floor independent of either input population.
            auto overhead = el::List<double>{};
            for (auto index = 0; index < 64 && !stopped(); ++index) {
                const auto before = _clock();
                const auto after = _clock();
                const auto ticks = static_cast<double>((after - before).toNanoseconds().toRawValue());
                if (ticks > 0) {
                    overhead.append(ticks);
                }
            }
            overhead.sort();
            const auto floor =
                overhead.isEmpty() ? 0.0 : overhead.getRefOrThrow(el::ItemIndex{overhead.count().toSizeT() / 2});
            const auto target = std::max(10000.0, floor * 100.0);
            test.clear();
            const auto prepareStart = _clock();
            if (!stopped()) {
                test.prepare(random, false);
            }
            if (!stopped()) {
                test.prepare(random, true);
            }
            const auto preparationCost =
                static_cast<double>((_clock() - prepareStart).toNanoseconds().toRawValue()) / 2.0;
            auto calibrated = false;
            auto measurementCost = 0.0;
            while (!stopped()) {
                const auto fixed = measure(0);
                if (stopped()) {
                    break;
                }
                const auto varying = measure(1);
                if (stopped()) {
                    break;
                }
                if (fixed >= target && varying >= target) {
                    calibrated = true;
                    measurementCost = std::max(fixed, varying);
                    break;
                }
                if (result._repetitions >= 1'048'576) {
                    break;
                }
                result._repetitions = el::saturatingMultiply(result._repetitions, uint64_t{2});
            }
            if (!calibrated) {
                result._detail = "Budget expired or timer could not resolve the operation."_el;
            } else {
                // Slow signing fixtures need small batches so setup cannot consume an entire budget before sampling.
                const auto batchCost = std::max(1.0, preparationCost + measurementCost);
                const auto boundedCount =
                    static_cast<uint64_t>(std::max(1.0, std::min(4096.0, 250'000'000.0 / batchCost)));
                batchSize = std::min(batchSize, boundedCount);
                auto pilot = el::List<double>{};
                test.clear();
                auto pilotPrepared = uint64_t{};
                for (; pilotPrepared < batchSize && !stopped(); ++pilotPrepared) {
                    test.prepare(random, random.getBool());
                }
                // An interrupted preparation never executes missing fixture indices.
                for (auto index = uint64_t{}; index < pilotPrepared && !stopped(); ++index) {
                    const auto value = measure(static_cast<std::size_t>(index));
                    if (value > 0 && std::isfinite(value)) {
                        pilot.append(value);
                    } else {
                        ++result._rejected;
                    }
                }
                if (!pilot.isEmpty() && !stopped()) {
                    statistics.initialize(std::move(pilot));
                    // Timing-dependent pilot sizes must not shift the seeded measurement inputs or labels.
                    auto measurementRandom = el::FastRandom{options.seed};
                    auto measured = uint64_t{};
                    while (!stopped() && (options.maximumSamples == 0 || measured < options.maximumSamples)) {
                        test.clear();
                        auto classes = el::List<uint8_t>{};
                        const auto count = options.maximumSamples == 0
                            ? batchSize
                            : std::min(batchSize, options.maximumSamples - measured);
                        for (auto index = uint64_t{}; index < count && !stopped(); ++index) {
                            const auto population = measurementRandom.getBool();
                            test.prepare(measurementRandom, population);
                            classes.append(static_cast<uint8_t>(population));
                        }
                        for (auto index = std::size_t{}; index < classes.count().toSizeT() && !stopped(); ++index) {
                            const auto population = classes.getRefOrThrow(el::ItemIndex{index}) != 0;
                            const auto value = measure(index);
                            ++measured;
                            if (value > 0 && std::isfinite(value)) {
                                statistics.add(value, population);
                            } else {
                                ++result._rejected;
                            }
                        }
                        result._evidence = statistics.evidence();
                        result._samples[0] = statistics.raw().population(false).count();
                        result._samples[1] = statistics.raw().population(true).count();
                        result._elapsed = _clock() - start;
                        if (result._evidence.eligible && std::abs(result._evidence.statistic) > 10.0) {
                            result._outcome = Outcome::Leakage;
                            result._detail =
                                el::StringFormat{
                                    "Threshold exceeded; reproduce with --test {} --seed {} --backend both; investigate the primitive in a focused follow-up."_el}
                                    .build(result._metadata.id, result._seed);
                            break;
                        }
                        if (progress) {
                            progress(result);
                        }
                    }
                    if (result._outcome != Outcome::Leakage && statistics.hasEnoughEvidence()) {
                        result._outcome = Outcome::NoLeakage;
                    }
                }
            }
        }
    } catch (const el::Exception &error) {
        result._outcome = Outcome::Error;
        result._detail = error.toString();
    } catch (const std::exception &error) {
        result._outcome = Outcome::Error;
        result._detail = el::String{error.what()};
    }
    if (result._outcome == Outcome::Insufficient && result._detail.isEmpty()) {
        result._detail = "Insufficient populations or degenerate variance within the budget."_el;
    }
    if (_interrupted != nullptr && _interrupted->load(std::memory_order_relaxed)) {
        result._outcome = Outcome::Interrupted;
    }
    result._elapsed = _clock() - start;
    test.clear();
    return result;
}

}
