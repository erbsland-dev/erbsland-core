// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Workload.hpp"

#include "impl/WorkloadTools.hpp"

namespace app::byte {

using namespace el::text::literals;
using namespace impl;

WorkloadRunner::WorkloadRunner(Configuration configuration) : _configuration{std::move(configuration)} {
}

void WorkloadRunner::printDryRun() const {
    for (const auto &scenario : _configuration.scenarios) {
        el::io::printLine(
            "record=scenario id="_el,
            scenario.id,
            " group="_el,
            scenario.group,
            " type="_el,
            toString(scenario.type),
            " use-case="_el,
            toString(scenario.useCase),
            " variant="_el,
            scenario.variant,
            " size-mode="_el,
            toString(scenario.sizeMode),
            " sensitive="_el,
            toString(scenario.sensitiveMode),
            " size="_el,
            scenario.size,
            " operand-size="_el,
            scenario.operandSize,
            " weight="_el,
            scenario.weight);
    }
    el::io::printLine(
        "record=summary action=dry-run scenarios="_el,
        _configuration.scenarios.size(),
        " threads="_el,
        _configuration.run.threadCount,
        " config-md5="_el,
        el::ByteFormat::compact(),
        configurationDigest(_configuration));
    el::stdOut()->flush();
}

void WorkloadRunner::printCoverage() const {
    for (const auto &descriptor : coverageRegistry()) {
        el::io::print(
            "record=coverage type="_el,
            toString(descriptor.type),
            " use-case="_el,
            toString(descriptor.useCase),
            " variant="_el,
            descriptor.variant,
            " apis="_el);
        for (auto index = std::size_t{}; index < descriptor.apiFamilies.size(); ++index) {
            if (index != 0U) {
                el::io::print(","_el);
            }
            el::io::print(descriptor.apiFamilies[index]);
        }
        el::io::printLine();
    }
    el::io::printLine("record=summary action=coverage paths="_el, coverageRegistry().size());
    el::stdOut()->flush();
}

auto WorkloadRunner::run() -> el::ExitCode {
    el::io::printLine(
        "record=run profile=byte-types mode="_el,
        toString(_configuration.run.mode),
        " workload-threads="_el,
        _configuration.run.threadCount,
        " seed="_el,
        _configuration.run.seed,
        " scenarios="_el,
        _configuration.scenarios.size(),
        " duration-ns="_el,
        _configuration.run.duration.count(),
        " sensitive-mode="_el,
        toString(_configuration.run.sensitiveSelection),
        " config-md5="_el,
        el::ByteFormat::compact(),
        configurationDigest(_configuration));
    const auto runStart = Clock::now();
    const auto deadline = runStart + _configuration.run.duration;
    auto nextProgress = runStart + _configuration.run.progressInterval;
    auto completed = std::size_t{};
    for (const auto &scenario : _configuration.scenarios) {
        if (Clock::now() >= deadline) {
            workloadError("The byte profiler deadline was reached before every scenario completed."_el);
        }
        const auto operations = calibrateOperations(_configuration, scenario);
        for (auto warmup = std::uint32_t{}; warmup < _configuration.run.warmupSamples; ++warmup) {
            runSample(_configuration, scenario, warmup, operations);
        }
        if (_configuration.run.mode == RunMode::Benchmark) {
            auto samples = std::vector<SampleResult>{};
            samples.reserve(_configuration.run.samples);
            for (auto sample = std::uint32_t{}; sample < _configuration.run.samples; ++sample) {
                if (Clock::now() >= deadline) {
                    workloadError(
                        "The benchmark deadline was reached before every scenario collected its required samples."_el);
                }
                samples.emplace_back(runSample(_configuration, scenario, sample + 100U, operations));
            }
            printBenchmark(scenario, samples);
        } else {
            for (auto repeat = std::uint32_t{}; repeat < scenario.weight && Clock::now() < deadline; ++repeat) {
                printSample(scenario, runSample(_configuration, scenario, repeat + 100U, operations));
            }
        }
        ++completed;
        if (Clock::now() >= nextProgress) {
            el::io::printLine(
                "record=progress completed="_el, completed, " total="_el, _configuration.scenarios.size());
            nextProgress = Clock::now() + _configuration.run.progressInterval;
        }
    }
    if (_configuration.run.mode == RunMode::Profile) {
        auto sample = std::uint64_t{1000U};
        while (Clock::now() < deadline) {
            for (const auto &scenario : _configuration.scenarios) {
                for (auto repeat = std::uint32_t{}; repeat < scenario.weight && Clock::now() < deadline; ++repeat) {
                    const auto operations = calibrateOperations(_configuration, scenario);
                    const auto result = runSample(_configuration, scenario, sample++, operations);
                    if (Clock::now() >= nextProgress) {
                        printSample(scenario, result);
                        nextProgress = Clock::now() + _configuration.run.progressInterval;
                    }
                }
            }
        }
    }
    el::io::printLine(
        "record=summary covered-scenarios="_el,
        completed,
        " total-scenarios="_el,
        _configuration.scenarios.size(),
        " elapsed-ns="_el,
        elapsedNanoseconds(runStart));
    el::stdOut()->flush();
    return el::ExitCode::success();
}

}
