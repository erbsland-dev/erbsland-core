// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConstantTimeApplication.hpp"

#include "RunResults.hpp"
#include "SelfTest.hpp"

#include <erbsland/core/ApplicationError.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/protected_data/ProtectedDataMode.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/cterm/TerminalDocumentRenderer.hpp>
#include <erbsland/err/Exception.hpp>
#include <erbsland/log/LogStream.hpp>
#include <erbsland/options/OptionEditor.hpp>
#include <erbsland/options/OptionManager.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionType.hpp>
#include <erbsland/options/OptionValues.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/system/CpuArchitecture.hpp>
#include <erbsland/system/impl/signal/ProcessSignalDispatcher.hpp>
#include <erbsland/system/OperatingSystem.hpp>
#include <erbsland/system/SystemInfo.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/time/TimePoint.hpp>
#include <erbsland/unit/ExitCode.hpp>
#include <erbsland/unit/Version.hpp>

#include <limits>

namespace app::constant_time {

using namespace el::text::literals;

void ConstantTimeApplication::initialize() {
    info().setApplicationName("Erbsland Core Constant-Time Tests"_el);
    info().setApplicationVersion(el::Version{1, 0, 0});
    enableTerminal();
    if (!terminal()->isInteractive()) {
        terminal()->setOutputMode(el::cterm::Terminal::OutputMode::BlockText);
    }
    cryptologyConfiguration().setProtectedDataMode(el::ProtectedDataMode::InternalOnly);
}

void ConstantTimeApplication::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpTitle("Constant-time leakage assessment"_el);
    options->setHelpDescription(
        "Run selected experiments sequentially. Results are evidence from this build and platform, not proof of constant time."_el);
    for (const auto &name : {"list-tests"_el, "all"_el, "self-test"_el}) {
        options->addOption({el::StringFormat{"--{}"_el}.build(name), name})
            .setType(el::OptionType::Flag)
            .setHelpDescription(name);
    }
    options->addOption({"--test"_el, "test"_el})
        .setType(el::OptionType::Text)
        .setMaximum(el::ArgumentCount::infinite())
        .setHelpDescription("Exact test ID; may be repeated."_el);
    options->addOption({"--backend"_el, "backend"_el})
        .setType(el::OptionType::Text)
        .setDefaultValue("both"_el)
        .setHelpDescription("portable, auto, or both"_el);
    options->addOption({"--duration-seconds"_el, "duration-seconds"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(int64_t{60})
        .setHelpDescription("Positive wall-time budget per test, including preparation."_el);
    options->addOption({"--max-samples"_el, "max-samples"_el})
        .setType(el::OptionType::Integer)
        .setHelpDescription("Optional positive measurement count limit."_el);
    options->addOption({"--seed"_el, "seed"_el})
        .setType(el::OptionType::Integer)
        .setHelpDescription("Nonnegative reproducible fixture seed."_el);
}

void ConstantTimeApplication::parseCommandLine() {
    try {
        Application::parseCommandLine();
    } catch (const el::Exception &error) {
        throw el::ApplicationError{error.toString(), el::ExitCode{2}};
    }
}

auto ConstantTimeApplication::main() -> el::ExitCode {
    const auto subscription = erbsland::system::impl::ProcessSignalDispatcher::instance().addSignal(
        [this](erbsland::system::impl::ProcessSignal signal, bool &claimed) -> void {
            if (signal == erbsland::system::impl::ProcessSignal::Interrupt ||
                signal == erbsland::system::impl::ProcessSignal::Terminate ||
                signal == erbsland::system::impl::ProcessSignal::ConsoleBreak) {
                _interrupted.store(true, std::memory_order_relaxed);
                claimed = true;
            }
        });
    try {
        return execute();
    } catch (const el::Exception &error) {
        logStream()->error(error.toString());
        return el::ExitCode{2};
    }
}

auto ConstantTimeApplication::execute() -> el::ExitCode {
    const auto &values = optionValues();
    const auto backend = values->getText("backend"_el);
    const auto duration = values->getInteger("duration-seconds"_el);
    const auto maximum = values->getInteger("max-samples"_el);
    const auto seed = values->getInteger("seed"_el);
    if ((backend != "portable"_el && backend != "auto"_el && backend != "both"_el) || duration <= 0 || seed < 0 ||
        (values->value("max-samples"_el) && maximum <= 0)) {
        throw el::ApplicationError{"Invalid backend, duration, sample limit, or seed."_el, el::ExitCode{2}};
    }
    if (values->getFlag("all"_el) && !values->getTextList("test"_el).isEmpty()) {
        throw el::ApplicationError{"--all and --test are mutually exclusive."_el, el::ExitCode{2}};
    }
    const auto registry = Registry::builtIn();
    const auto selected = registry.select(values->getTextList("test"_el), values->getFlag("all"_el));
    auto runner = Runner{el::TimePoint::now, &_interrupted};
    if (values->getFlag("self-test"_el)) {
        SelfTest{}.checkDeterministic();
        printRecord("record=self-test deterministic=success"_el);
        if (!ERBSLAND_CONSTANT_TIME_OPTIMIZED) {
            printRecord("record=self-test control=unavailable reason=optimized-build-required"_el);
            return el::ExitCode{3};
        }
        printBuild(1234567);
        const auto control = SelfTest{}.runControl(runner);
        printRecord(control.toString());
        return el::ExitCode{
            control.outcome() == Outcome::Leakage           ? 0
                : control.outcome() == Outcome::Interrupted ? 130
                : control.outcome() == Outcome::Error       ? 2
                                                            : 3};
    }
    if (values->getFlag("list-tests"_el)) {
        cryptologyConfiguration().setHardwareAccelerationEnabled(true);
        for (const auto &entry : registry.entries()) {
            const auto test = entry.create(true);
            const auto &metadata = test->metadata();
            printRecord(
                el::StringFormat{"record=test id={} description={} populations={} backend={} variants={}"_el}.build(
                    metadata.id,
                    metadata.description,
                    metadata.populations,
                    metadata.backend,
                    metadata.backendVariants ? "portable,auto"_el : "portable"_el));
        }
        return el::ExitCode::success();
    }
    if (selected.isEmpty()) {
        el::cterm::TerminalDocumentRenderer{}.renderTo(*terminal(), el::OptionManager{options()}.helpDocument({}));
        return el::ExitCode::success();
    }
    if (!ERBSLAND_CONSTANT_TIME_OPTIMIZED) {
        throw el::ApplicationError{"Timing experiments require a Release or RelWithDebInfo build."_el, el::ExitCode{2}};
    }
    const auto actualSeed = values->value("seed"_el)
        ? static_cast<uint64_t>(seed)
        : random().getUInt64(0, static_cast<uint64_t>(std::numeric_limits<int64_t>::max()));
    printBuild(actualSeed);
    const auto options = RunOptions{
        .duration = el::Seconds{duration}, .maximumSamples = static_cast<uint64_t>(maximum), .seed = actualSeed};
    auto results = RunResults{};
    for (const auto &entry : selected) {
        auto previousBackend = el::String{};
        for (const auto automatic : {false, true}) {
            if ((backend == "portable"_el && automatic) || (backend == "auto"_el && !automatic)) {
                continue;
            }
            cryptologyConfiguration().setHardwareAccelerationEnabled(automatic);
            try {
                auto runOptions = options;
                runOptions.start = el::TimePoint::now();
                auto test = entry.create(automatic);
                if (automatic && !previousBackend.isEmpty() &&
                    (!test->metadata().backendVariants || previousBackend == test->metadata().backend)) {
                    printRecord(
                        el::StringFormat{
                            "record=backend test={} outcome=unavailable reason=no-distinct-automatic-backend actual={}"_el}
                            .build(entry.id(), test->metadata().backend));
                    continue;
                }
                previousBackend = test->metadata().backend;
                auto nextProgress = el::TimePoint::now() + el::Seconds{2};
                auto result = runner.run(*test, runOptions, [this, &nextProgress](const Result &progress) -> void {
                    if (terminal()->isInteractive() && el::TimePoint::now() >= nextProgress) {
                        printRecord(
                            el::StringFormat{"Progress {} samples={}"_el}.build(
                                progress.metadata().id, progress.samples(false) + progress.samples(true)));
                        nextProgress = el::TimePoint::now() + el::Seconds{2};
                    }
                });
                printRecord(result.toString());
                results.append(std::move(result));
            } catch (const el::Exception &error) {
                auto result = Result{TestMetadata{.id = entry.id()}, Outcome::Error, actualSeed};
                result.setDetail(error.toString());
                printRecord(result.toString());
                results.append(std::move(result));
            }
            if (_interrupted.load(std::memory_order_relaxed)) {
                return el::ExitCode{130};
            }
        }
    }
    const auto exit = results.exitCode();
    printRecord(el::StringFormat{"record=summary experiments={} exit-code={}"_el}.build(results.count(), exit));
    return el::ExitCode{exit};
}

void ConstantTimeApplication::printBuild(const uint64_t seed) const {
    printRecord(
        el::StringFormat{"record=run build={} compiler={} os={} cpu={} seed={}"_el}.build(
            el::String{ERBSLAND_CONSTANT_TIME_BUILD_TYPE},
            el::String{ERBSLAND_CONSTANT_TIME_COMPILER},
            el::system::info::operatingSystem().toString(),
            el::system::info::cpuArchitecture().toString(),
            seed));
}

void ConstantTimeApplication::printRecord(const el::String &line) const {
    terminal()->printLine(line);
    terminal()->flush();
}

}
