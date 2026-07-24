// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Application_fwd.hpp"
#include "Configuration.hpp"
#include "DefaultConfiguration.hpp"
#include "Workload.hpp"

#include <erbsland/profiling/ProfilingApplication.hpp>

#include <algorithm>
#include <optional>

namespace app::stream {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

/// The concurrent file-stream profiler command-line application.
/// @notest{Covered by the registered profiler CTest entries.}
class StreamFileProfileApplication final : public pf::ProfilingApplication {
public:
    using ProfilingApplication::ProfilingApplication;

protected: // implement pf::ProfilingApplication
    void configureProfiling(pf::ProfilingDefinition &definition) override {
        definition
            .setApplication(
                "File Stream Profiling"_el,
                el::Version{1, 0, 0},
                "Concurrent File Stream Profiler and Benchmark"_el,
                "Profiles deterministic binary and encoded-text file stream workloads with independent concurrent "
                "callers. With no options, the four-thread cover-all profile runs for eight minutes."_el)
            .setDefaultConfiguration(el::String{cDefaultConfigurationText});
    }

    void registerProfilingOptions(const el::OptionsPtr &options) override {
        options->addOption({"--keep-files"_el, "keep-files"_el})
            .setHelpDescription("Keep the generated temporary workspace after the run."_el);
    }

    [[nodiscard]] auto executeProfiling() -> el::ExitCode override {
        const auto values = optionValues();
        if (!values->valueCount("write-template"_el).isZero()) {
            const auto path = el::Path{values->getText("write-template"_el)};
            ConfigurationLoader::writeTemplate(path);
            el::io::printLine("template="_el, path.toString());
            return el::ExitCode::success();
        }

        auto configPath = std::optional<el::Path>{};
        if (!values->valueCount("config"_el).isZero()) {
            configPath = el::Path{values->getText("config"_el)};
        }
        auto configuration = ConfigurationLoader::load(configPath);
        if (!values->valueCount("mode"_el).isZero()) {
            configuration.run.mode =
                values->getText("mode"_el) == "benchmark"_el ? RunMode::Benchmark : RunMode::Profile;
        }
        if (!values->valueCount("threads"_el).isZero()) {
            const auto count = values->getInteger("threads"_el);
            if (count < 1 || count > 256) {
                throw el::ApplicationError{"The workload thread count must be in the range 1-256."_el};
            }
            configuration.run.threadCount = static_cast<std::uint32_t>(count);
        }
        if (values->getFlag("keep-files"_el)) {
            configuration.run.keepFiles = true;
        }
        const auto filters = values->getTextList("scenario"_el);
        if (!filters.empty()) {
            std::erase_if(configuration.scenarios, [&](const Scenario &scenario) -> bool {
                return std::ranges::none_of(filters, [&](const el::String &filter) -> bool {
                    return scenario.id == filter || scenario.group == filter;
                });
            });
            if (configuration.scenarios.empty()) {
                throw el::ApplicationError{"No expanded scenario matches the requested filter."_el};
            }
        }

        if (values->getFlag("list-scenarios"_el)) {
            for (const auto &scenario : configuration.scenarios) {
                el::io::printLine("record=scenario id="_el, scenario.id);
            }
            return el::ExitCode::success();
        }
        auto runner = WorkloadRunner{std::move(configuration)};
        if (values->getFlag("dry-run"_el)) {
            runner.printDryRun();
            return el::ExitCode::success();
        }
        return runner.run();
    }
};

}
