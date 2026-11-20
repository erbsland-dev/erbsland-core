// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InboxMonitorApp.hpp"

#include <erbsland/EventTarget.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/StandardStreams.hpp>

namespace demo {

using namespace el::text::literals;

void InboxMonitorApp::initialize() {
    info().setApplicationName("Inbox Monitor"_el);
}

void InboxMonitorApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpDescription("Report newly visible files in an inbox directory."_el);
    options->addOption(cDirectory)
        .setRequired()
        .setHelpTitle("Inbox directory"_el)
        .setHelpDescription("Watch regular files directly inside this directory."_el);
    options->addOption({"--once"_el, cOnce})
        .setType(el::OptionType::Flag)
        .setHelpTitle("Scan once"_el)
        .setHelpDescription("Print the initial arrivals and exit."_el);
}

void InboxMonitorApp::handleCommandLineOptions(const el::OptionValuesPtr &values) {
    const auto directory = el::Path::fromNativeOrThrow(values->getText(cDirectory)).toAbsoluteOrThrow();
    if (!directory.info().isDirectory()) {
        throw el::OptionError{"Expected an existing inbox directory."_el, values->value(cDirectory)};
    }
    const auto once = values->getFlag(cOnce);
    loop()->invoke([this, directory, once]() -> void { watch(directory, once); });
}

void InboxMonitorApp::watch(const el::Path &directory, const bool once) {
    _watcher = InboxWatcher::create(directory);
    _watcher->events().onFileArrived([this](const el::Path &file) -> void {
        ++_arrivals;
        el::io::printLine("Arrived  "_el, file.name());
        el::stdOut()->flush();
    });
    _watcher->start();
    if (once) {
        _watcher->stop();
        el::io::printLine("Scan complete. Arrivals: "_el, _arrivals);
        quit();
    }
}

}
