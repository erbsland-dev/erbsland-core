// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileActivityMonitorApp.hpp"

#include <DemoTerminal.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathInfoParts.hpp>
#include <erbsland/PathResolveMode.hpp>
#include <erbsland/StandardStreams.hpp>

#include <initializer_list>

namespace demo {

using namespace el::text::literals;

void FileActivityMonitorApp::initialize() {
    DemoTerminal::enable(*this);
    info().setApplicationName("File Activity Monitor"_el);
}

void FileActivityMonitorApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpDescription("Report file size or modification-time changes until the file is quiet."_el);
    options->addOption("file"_el).setRequired().setHelp("An existing regular file to monitor."_el);
    options->addOption({"--interval-ms"_el, "interval-ms"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(250)
        .setHelp("Positive polling interval in milliseconds."_el);
    options->addOption({"--idle-timeout"_el, "idle-timeout"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(5)
        .setHelp("Stop after this many seconds without an observed change; must be positive."_el);
    options->addOption({"--timeout"_el, "timeout"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(0)
        .setHelp("Overall monitoring limit in seconds; zero disables this limit."_el);
}

void FileActivityMonitorApp::handleCommandLineOptions(const el::OptionValuesPtr &values) {
    const auto interval = values->getInteger("interval-ms"_el);
    const auto idleTimeout = values->getInteger("idle-timeout"_el);
    const auto timeout = values->getInteger("timeout"_el);
    if (interval <= 0) {
        throw el::OptionError{"The polling interval must be positive."_el, values->value("interval-ms"_el)};
    }
    if (idleTimeout <= 0) {
        throw el::OptionError{"The inactivity timeout must be positive."_el, values->value("idle-timeout"_el)};
    }
    if (timeout < 0) {
        throw el::OptionError{"The overall timeout must be non-negative."_el, values->value("timeout"_el)};
    }
    _interval = el::Milliseconds{interval};
    _idleTimeout = el::Seconds{idleTimeout};
    _timeout = el::Seconds{timeout};
    const auto path =
        el::Path::fromNativeOrThrow(values->getText("file"_el)).resolveOrThrow(el::PathResolveMode::Physical);
    _fileInfo = path.info();
    if (!_fileInfo.isRegularFile()) {
        throw el::OptionError{"Expected an existing regular file."_el, values->value("file"_el)};
    }
    loop()->invoke([this]() -> void { begin(); });
}

void FileActivityMonitorApp::begin() {
    // Establish the initial snapshot before starting the quiet period.
    poll();
    _idleTimer = loop()->createTimer([this]() -> void { finish("Inactivity timeout reached."_el); });
    _idleTimer->startOnce(_idleTimeout);
    _pollTimer = loop()->createTimer([this]() -> void { poll(); });
    _pollTimer->startFixedRate(_interval);
    if (_timeout.isPositive()) {
        _deadline = loop()->createTimer([this]() -> void { finish("Overall deadline reached."_el); });
        _deadline->startOnce(_timeout);
    }
    el::stdOut()->printLine("Monitoring file: "_el, _fileInfo.path());
    el::stdOut()->flush();
}

void FileActivityMonitorApp::poll() {
    _fileInfo.reload(el::PathInfoParts{el::PathInfoPart::Type, el::PathInfoPart::Size, el::PathInfoPart::Times});
    if (!_fileInfo.isRegularFile()) {
        throw el::ApplicationError{"The monitored file is unavailable or no longer a regular file."_el};
    }
    const auto size = _fileInfo.fileSize();
    const auto modification = _fileInfo.lastModified();
    const auto changed = size != _previousSize || modification != _previousModification;
    _previousSize = size;
    _previousModification = modification;
    // The initial snapshot has no activity timer yet.
    if (changed && _idleTimer) {
        onActivity();
    }
}

void FileActivityMonitorApp::onActivity() {
    // Only an observed change restarts the quiet period; the overall deadline stays unchanged.
    _idleTimer->startOnce(_idleTimeout);
    el::stdOut()->printLine("Changed: "_el, _previousSize, " bytes; modified "_el, _previousModification.toString());
    el::stdOut()->flush();
}

void FileActivityMonitorApp::finish(const el::String &reason) {
    stopTimers();
    el::stdOut()->printLine(reason);
    el::stdOut()->flush();
    quit();
}

void FileActivityMonitorApp::stopTimers() noexcept {
    for (const auto &timer : {_pollTimer, _idleTimer, _deadline}) {
        if (timer) {
            timer->stop();
        }
    }
}

void FileActivityMonitorApp::cleanup() noexcept {
    stopTimers();
    _pollTimer.reset();
    _idleTimer.reset();
    _deadline.reset();
}

}
