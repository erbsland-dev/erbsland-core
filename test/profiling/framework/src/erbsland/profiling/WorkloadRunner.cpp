// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "WorkloadRunner.hpp"

#include "ScenarioWorkload.hpp"
#include "Seed.hpp"
#include "Statistics.hpp"
#include "SteadyTimeSource.hpp"
#include "TimeSource.hpp"
#include "WorkerExecutionContext.hpp"
#include "WorkerWorkload.hpp"

#include <algorithm>
#include <barrier>
#include <exception>
#include <memory>
#include <stop_token>
#include <thread>
#include <vector>

namespace erbsland::profiling {

using namespace text::literals;

WorkloadRunner::WorkloadRunner(
    const ProfilingDefinition &definition, ProfilingConfiguration configuration, TimeSourcePtr timeSource) :
    _definition{definition},
    _configuration{std::move(configuration)},
    _timeSource{timeSource ? std::move(timeSource) : std::make_shared<SteadyTimeSource>()} {
}

void WorkloadRunner::printDryRun() const {
    for (const auto &scenario : _configuration.scenarios) {
        io::printLine(
            "record=scenario id="_el,
            scenario.id,
            " group="_el,
            scenario.group,
            " functionality="_el,
            scenario.functionality,
            " weight="_el,
            scenario.weight);
    }
    io::printLine(
        "record=summary action=dry-run scenarios="_el,
        _configuration.scenarios.count().toRawValue(),
        " threads="_el,
        _configuration.run.threadCount,
        " config-md5="_el,
        ByteFormat::compact(),
        _configuration.digest);
    stdOut()->flush();
}

void WorkloadRunner::printCoverage() const {
    for (const auto &functionality : _definition.functionalities()) {
        io::print("record=coverage functionality="_el, functionality.id, " api="_el);
        auto separator = ""_el;
        for (const auto &api : functionality.api) {
            io::print(separator, api);
            separator = ","_el;
        }
        io::printLine(" description="_el, functionality.description);
    }
    io::printLine(
        "record=summary action=coverage functionalities="_el, _definition.functionalities().count().toRawValue());
    stdOut()->flush();
}

auto WorkloadRunner::runSample(
    ScenarioWorkload &workload, const Scenario &scenario, const std::uint64_t sample, const std::uint64_t operations)
    -> SampleMeasurement {
    const auto threadCount = _configuration.run.threadCount;
    auto workers = List<WorkerWorkloadPtr>{};
    workers.reserve(ItemCount{threadCount});
    for (auto worker = std::uint32_t{}; worker < threadCount; ++worker) {
        workers.append(workload.createWorker(worker));
    }
    auto results = std::make_unique<WorkerMeasurement[]>(threadCount);
    auto failures = std::make_unique<std::exception_ptr[]>(threadCount);
    auto stopSource = std::stop_source{};
    auto startBarrier = std::barrier{static_cast<std::ptrdiff_t>(threadCount + 1U)};
    auto finishBarrier = std::barrier{static_cast<std::ptrdiff_t>(threadCount + 1U)};
    auto threads = std::vector<std::jthread>{}; // Move-only native thread ownership is the deliberate List exception.
    threads.reserve(threadCount);
    for (auto worker = std::uint32_t{}; worker < threadCount; ++worker) {
        threads.emplace_back([&, worker]() -> void {
            try {
                startBarrier.arrive_and_wait();
                auto timer = ElapsedTimer{};
                results[worker] = workers.toRawValue()[worker]->execute(
                    WorkerExecutionContext{
                        .operations = operations,
                        .seed = deriveSeed(_configuration.run.seed, scenario.id, worker, sample),
                        .stopToken = stopSource.get_token()});
                results[worker].elapsed = timer.elapsed();
            } catch (...) {
                failures[worker] = std::current_exception();
                stopSource.request_stop();
            }
            finishBarrier.arrive_and_wait();
        });
    }
    startBarrier.arrive_and_wait();
    auto wallTimer = ElapsedTimer{};
    finishBarrier.arrive_and_wait();
    auto measurement = SampleMeasurement{.wallTime = wallTimer.elapsed()};
    for (auto worker = std::uint32_t{}; worker < threadCount; ++worker) {
        if (failures[worker]) {
            std::rethrow_exception(failures[worker]);
        }
        measurement.operations += results[worker].operations;
        measurement.workers.append(results[worker]);
    }
    const auto metricCount = _definition.metrics().count().toSizeT();
    measurement.metrics.resize(ItemCount{metricCount}, 0U);
    for (const auto &worker : measurement.workers) {
        if (worker.metrics.count().toSizeT() != metricCount) {
            throw ApplicationError{"A workload returned an unexpected number of metrics."_el};
        }
        for (auto index = std::size_t{}; index < metricCount; ++index) {
            measurement.metrics.set(
                ItemIndex{index}, measurement.metrics.toRawValue()[index] + worker.metrics.toRawValue()[index]);
        }
    }
    workload.validate(measurement);
    return measurement;
}

auto WorkloadRunner::calibrate(ScenarioWorkload &workload, const Scenario &scenario) -> std::uint64_t {
    auto operations = std::uint64_t{1U};
    while (operations < workload.maximumOperations()) {
        const auto sample = runSample(workload, scenario, 0U, operations);
        if (sample.wallTime >= _configuration.run.minimumSampleTime) {
            return operations;
        }
        operations = std::min(workload.maximumOperations(), operations * 2U);
    }
    return operations;
}

void WorkloadRunner::printSample(const Scenario &scenario, const SampleMeasurement &measurement) const {
    io::print(
        "record=sample scenario="_el,
        scenario.id,
        " operations="_el,
        measurement.operations,
        " wall-ns="_el,
        measurement.wallTime.toNanoseconds().toRawValue());
    io::print(" worker-fairness="_el, Statistics::workerFairness(measurement.workers));
    for (auto index = std::size_t{}; index < _definition.metrics().count().toSizeT(); ++index) {
        const auto &metric = _definition.metrics().toRawValue()[index];
        const auto value = measurement.metrics.toRawValue()[index];
        io::print(" "_el, metric.id, "="_el, value);
        if (metric.reportRate) {
            io::print(" "_el, metric.id, "-per-second="_el, Statistics::normalizedRate(value, measurement.wallTime));
        }
    }
    io::printLine(""_el);
}

void WorkloadRunner::printBenchmark(const Scenario &scenario, const List<SampleMeasurement> &measurements) const {
    const auto statistics = Statistics{measurements};
    io::print(
        "record=benchmark scenario="_el,
        scenario.id,
        " samples="_el,
        measurements.count().toRawValue(),
        " min-ns-per-operation="_el,
        statistics.minimum(),
        " median-ns-per-operation="_el,
        statistics.median(),
        " mean-ns-per-operation="_el,
        statistics.mean(),
        " p95-ns-per-operation="_el,
        statistics.p95(),
        " max-ns-per-operation="_el,
        statistics.maximum());
    io::print(" worker-fairness="_el, statistics.workerFairness());
    for (auto index = std::size_t{}; index < _definition.metrics().count().toSizeT(); ++index) {
        const auto &metric = _definition.metrics().toRawValue()[index];
        const auto value = statistics.metricTotal(index);
        io::print(" "_el, metric.id, "="_el, value);
        if (metric.reportRate) {
            io::print(
                " "_el, metric.id, "-per-second="_el, Statistics::normalizedRate(value, statistics.totalElapsed()));
        }
    }
    io::printLine(""_el);
}

auto WorkloadRunner::run() -> ExitCode {
    io::printLine(
        "record=run mode="_el,
        _configuration.run.mode == RunMode::Benchmark ? "benchmark"_el : "profile"_el,
        " threads="_el,
        _configuration.run.threadCount,
        " scenarios="_el,
        _configuration.scenarios.count().toRawValue(),
        " config-md5="_el,
        ByteFormat::compact(),
        _configuration.digest);
    const auto startTime = _timeSource->now();
    const auto elapsed = [&]() noexcept -> TimeDelta { return _timeSource->now() - startTime; };
    auto nextProgress = _configuration.run.progressInterval;
    for (const auto &scenario : _configuration.scenarios) {
        io::printLine(
            "record=scenario id="_el,
            scenario.id,
            " group="_el,
            scenario.group,
            " functionality="_el,
            scenario.functionality,
            " weight="_el,
            scenario.weight);
        const auto *functionality = _definition.findFunctionality(scenario.functionality);
        if (functionality == nullptr || !functionality->factory) {
            throw ApplicationError{
                StringFormat{"No workload is registered for '{}'."_el}.build(scenario.functionality)};
        }
        auto workload = functionality->factory(scenario);
        workload->prepare(_configuration.run, scenario);
        const auto operations = calibrate(*workload, scenario);
        for (auto warmup = std::uint32_t{}; warmup < _configuration.run.warmupSamples; ++warmup) {
            runSample(*workload, scenario, warmup + 1U, operations);
        }
        if (_configuration.run.mode == RunMode::Benchmark) {
            auto measurements = List<SampleMeasurement>{};
            for (auto sample = std::uint32_t{}; sample < _configuration.run.samples; ++sample) {
                if (elapsed() >= _configuration.run.duration) {
                    throw ApplicationError{"The profiling deadline was reached before all samples completed."_el};
                }
                measurements.append(runSample(*workload, scenario, sample + 100U, operations));
                if (elapsed() >= nextProgress) {
                    io::printLine(
                        "record=progress scenario="_el,
                        scenario.id,
                        " elapsed-ns="_el,
                        elapsed().toNanoseconds().toRawValue());
                    nextProgress += _configuration.run.progressInterval;
                }
            }
            printBenchmark(scenario, measurements);
        } else {
            auto sample = std::uint64_t{100U};
            while (elapsed() < _configuration.run.duration) {
                for (auto repeat = std::uint32_t{}; repeat < scenario.weight; ++repeat) {
                    printSample(scenario, runSample(*workload, scenario, sample++, operations));
                }
                if (elapsed() >= nextProgress) {
                    io::printLine(
                        "record=progress scenario="_el,
                        scenario.id,
                        " elapsed-ns="_el,
                        elapsed().toNanoseconds().toRawValue());
                    nextProgress += _configuration.run.progressInterval;
                }
            }
        }
    }
    io::printLine("record=summary elapsed-ns="_el, elapsed().toNanoseconds().toRawValue());
    stdOut()->flush();
    return ExitCode::success();
}

}
