// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoTerminal.hpp>
#include <erbsland/all_core.hpp>
#include <erbsland/all_err.hpp>
#include <erbsland/all_event.hpp>
#include <erbsland/all_options.hpp>
#include <erbsland/all_path.hpp>
#include <erbsland/all_stream.hpp>
#include <erbsland/all_text.hpp>
#include <erbsland/all_time.hpp>

namespace demo {

using namespace el::text::literals;

struct AppData {
    el::EventTimerPtr pollTimer;
    el::PathInfo pathInfo;
} appData;

/// This method is called in configured intervals to display the file size.
void poll() {
    appData.pathInfo.reload();
    if (!appData.pathInfo.exists()) {
        el::stdOut()->printLine("File not found"_el);
    } else {
        el::stdOut()->printLine("Size: "_el, appData.pathInfo.fileSize(), " bytes"_el);
    }
}

/// In an event-based application, you initialize your event handlers and let the event loop run.
void initialize() {
    el::stdOut()->printLine("Monitoring size of file: "_el, appData.pathInfo.resolvedPath());
    poll();
    appData.pollTimer = el::application().loop()->createTimer(poll);
    appData.pollTimer->startFixedDelay(el::Seconds{1});
}

/// This method is automatically called after the options are parsed.
void handleOptions(const el::OptionValuesPtr &values) {
    try {
        appData.pathInfo = el::Path::fromNativeOrThrow(values->getText("file"_el)).resolveOrThrow().info();
    } catch (const el::ParseError &) {
        throw el::ApplicationError{"The path has an invalid format"_el, std::current_exception()};
    } catch (const el::PathError &) {
        throw el::ApplicationError{
            "The file at the given path does not exist or is no regular file"_el, std::current_exception()};
    }
    const auto timeout = el::Seconds{values->getInteger("timeout"_el)};
    if (timeout.isNegative()) {
        throw el::ApplicationError{"The timeout must be a non-negative integer."_el};
    }
    el::application().loop()->invoke(initialize);
    if (timeout.isPositive()) {
        el::application().loop()->invokeAfter(timeout, []() -> void { el::application().quit(); });
    }
}

/// Here you create the command line options.
auto createOptions() -> el::OptionSetPtr {
    auto optionSet = el::OptionSet::create();
    optionSet->addOption("file"_el).setRequired().setHelp("The path to the file to monitor."_el);
    optionSet->addOption({"-t"_el, "--timeout"_el, "timeout"_el})
        .setType(el::OptionType::Integer)
        .setHelp("The timeout in seconds. Zero for no timeout."_el)
        .setDefaultValue(5);
    optionSet->setPostParsingFn(handleOptions);
    return optionSet;
}

/// This simple main method is the entry point of the application.
auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    demo::DemoTerminal::enable(app);
    app.info().setApplicationName("File Size Monitor"_el);
    app.options()->addSet(createOptions());
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
