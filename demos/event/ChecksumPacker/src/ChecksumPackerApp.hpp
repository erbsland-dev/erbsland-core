// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constants.hpp"
#include "Preparation.hpp"
#include "Workflow.hpp"

#include <DemoTerminal.hpp>
#include <erbsland/all_options.hpp>
#include <erbsland/all_path.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/Events.hpp>
#include <erbsland/List.hpp>
#include <erbsland/LogConfiguration.hpp>
#include <erbsland/LogLineFormat.hpp>
#include <erbsland/LogManager.hpp>
#include <erbsland/LogWriter.hpp>
#include <erbsland/OptionFlag.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/Version.hpp>

#include <exception>
#include <memory>
#include <utility>

namespace demo {

using namespace el::text::literals;

/// Pack source directories through a scan/pack workflow coordinated by the main event loop.
/// Worker results cross the event queue; application ownership lasts until all workers are joined.
class ChecksumPackerApp final : public el::Application {
public:
    // use the constructors from el::Application
    using Application::Application;

protected: // implement el::Application
    void initialize() override {
        DemoTerminal::enable(*this);
        info().setApplicationName("Checksum Packer"_el);
        info().setApplicationVersion(el::Version(1, 0, 0));
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpDescription("Create one ZIP per source directory with a SHA3-256 'checksum.txt' manifest."_el);
        options->addOption({cSourceDirectoryOption})
            .setType(el::OptionType::Text)                       // we need text arguments for the paths.
            .setFlag(el::OptionFlag::Required)                   // at least one directory needs to be specified
            .setHelpTitle("One or more directories to pack."_el) // help text displayed on `--help`.
            .setValueName("directory"_el)                        // a more speaking name for the help.
            .setMaximum(el::ArgumentCount{16});                  // maximum number of directories to pack.
        options->addOption({"-o"_el, "--output"_el, cOutputDirectoryOption})
            .setType(el::OptionType::Text)                       // we use a text value for the output path
            .setHelpTitle("Output directory path."_el)           // help text displayed on `--help`.
            .setValueName("directory"_el);                       // a more speaking name for the help.
        options->addOption({"-v"_el, "--verbose"_el, cVerboseLoggingOption})
            .setType(el::OptionType::Flag)                       // a simple flag
            .setHelpTitle("Enables verbose logging."_el);        // help text in `--help`.
    }
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        setupLogging();
        _preparation.prepare(values);
        startWorkflows();
    }
    void cleanup() noexcept override {
        for (const auto &workflow : _workflows) {
            workflow->shutdown();
        }
    }

private:
    /// Setup logging
    void setupLogging() {
        auto logConfiguration = el::LogConfiguration{};
        auto logLineFormat = el::LogLineFormat{}.setPattern("{name} | {message}"_el);
        logConfiguration.setLineFormat(logLineFormat);
        logConfiguration.addWriter(el::LogWriter::createForConsole(el::application().terminal()));
        if (el::application().optionValues()->getFlag(cVerboseLoggingOption)) {
            logConfiguration.enableTraceSection(cVerboseTraceSection);
        }
        el::application().log().setConfiguration(logConfiguration);
    }
    /// Start all workflows.
    void startWorkflows() {
        logStream()->info("Checksum Packer V", info().applicationVersion().toString());
        // process each source directory individually and start a new workflow for it.
        for (const auto &[source, zipFile, logPath] : _preparation.sourceAndZipFiles()) {
            auto logStream = log().createStream(logPath, cVerboseTraceSection);
            // create a new workflow and schedule its start.
            auto workflow = std::make_shared<Workflow>(
                source, zipFile, std::move(logStream), [this](std::exception_ptr error) -> void {
                    this->onWorkflowDone(std::move(error));
                });
            _workflows.append(workflow);
            events()->invoke([workflow = std::move(workflow)]() -> void { workflow->start(); });
        }
    }
    /// Event called when a workflow finishes.
    void onWorkflowDone(std::exception_ptr error) {
        if (error != nullptr) {
            // rethrow the original exception.
            std::rethrow_exception(std::move(error));
        }
        // if one workflow successfully finished, test if all workflows are done.
        for (const auto &workflow : _workflows) {
            if (!workflow->isDone()) {
                return; // wait for the other workflows to finish.
            }
        }
        logStream()->info("All packages were created successfully."_el);
        quit();
    }

private:
    Preparation _preparation;         ///< The preparation part of the application.
    el::List<el::Path> _zipPaths;     ///< The paths for the final ZIP files.
    el::List<WorkflowPtr> _workflows; ///< The list of all created workflows.
};

}
