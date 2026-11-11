// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LookupProgress.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ApplicationInfo.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/HostLookup.hpp>
#include <erbsland/HostLookupEventEditor.hpp>
#include <erbsland/IpAddress.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/NetworkErrorContext.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/TimeDelta.hpp>

#include <memory>

namespace demo {

using namespace el::text::literals;

/// Keep the lookup result even when its optional progress reporter disappears.
class LookupOptionalRecipientApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override { info().setApplicationName("Lookup with Optional Progress"_el); }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->addOption("host"_el)
            .setDefaultValue("127.0.0.1"_el)
            .setHelp("Server name or IP address to resolve."_el);
        options->addOption({"--timeout-ms"_el, "timeout"_el})
            .setType(el::OptionType::Integer)
            .setDefaultValue(1000)
            .setHelp("Application deadline in milliseconds; zero aborts at the end of setup."_el);
        options->addOption({"--quiet"_el, "quiet"_el}).setHelp("Disable progress output; still print the result."_el);
    }
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        _host = el::Host::fromStringOrThrow(values->getText("host"_el));
        const auto timeout = values->getInteger("timeout"_el);
        if (timeout < 0) {
            throw el::OptionError{"The deadline must be non-negative."_el, values->value("timeout"_el)};
        }
        _timeout = el::TimeDelta::milliseconds(timeout);
        _quiet = values->getFlag("quiet"_el);
        loop()->invoke([this]() -> void { begin(); });
    }
    void cleanup() noexcept override {
        _lookup.reset();
        _progress.reset();
    }

private:
    void begin() {
        loop()->invokeAfter(_timeout, [this]() -> void { onTimeout(); });
        _lookup = loop()->get<el::Network>().createHostLookup();
        _progress = std::make_shared<LookupProgress>(_host);
        _lookup->events()
            .onResolved(
                [this, weak = std::weak_ptr<LookupProgress>{_progress}](
                    const el::List<el::IpAddress> &addresses) -> void {
                    _addresses = addresses; // Keep the required result independently of progress output.
                    if (const auto progress = weak.lock(); progress != nullptr) {
                        progress->onResolved(addresses);
                    }
                })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                el::io::printLine("Lookup failed: "_el, error.title());
                _exitCode = el::ExitCode::failure();
            })
            .onFinal([this]() -> void {
                _finished = true;
                _lookup.reset();
                for (const auto &address : _addresses) {
                    el::io::printLine("Server address: "_el, address);
                }
                quit(_exitCode); // Completion never depends on the progress reporter.
            });
        _lookup->start(_host);
        if (_quiet) {
            // Turn off an already connected reporter before any result can be dispatched.
            _progress.reset();
        }
        if (_timeout.isZero()) {
            onTimeout();
        }
    }
    void onTimeout() {
        if (_finished) {
            return;
        }
        _finished = true;
        _lookup.reset();
        el::io::printLine("Deadline reached without a lookup result."_el);
        quit(el::ExitCode::failure());
    }

private:
    el::HostLookupPtr _lookup;                       ///< Required operation, owned by the application.
    el::List<el::IpAddress> _addresses;              ///< Result copied out of the callback's borrowed argument.
    std::shared_ptr<LookupProgress> _progress;       ///< Optional reporter that can be released independently.
    bool _quiet{false};                              ///< Whether to discard progress reporting after setup.
    el::Host _host;                                  ///< Server selected on the command line.
    el::TimeDelta _timeout;                          ///< Independent application deadline.
    el::ExitCode _exitCode{el::ExitCode::success()}; ///< Result reported when the operation finishes.
    bool _finished{false};                           ///< Prevents a late deadline from changing a completed result.
};

}
