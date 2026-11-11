// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

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

namespace demo {

using namespace el::text::literals;

/// Resolve a configured server address with intentionally missing source ownership.
class LookupLostSourceApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override { info().setApplicationName("Server Address Lookup"_el); }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->addOption("host"_el)
            .setDefaultValue("127.0.0.1"_el)
            .setHelp("Server name or IP address to resolve."_el);
        options->addOption({"--timeout-ms"_el, "timeout"_el})
            .setType(el::OptionType::Integer)
            .setDefaultValue(1000)
            .setHelp("Application deadline in milliseconds; zero aborts at the end of setup."_el);
    }
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        _host = el::Host::fromStringOrThrow(values->getText("host"_el));
        const auto timeout = values->getInteger("timeout"_el);
        if (timeout < 0) {
            throw el::OptionError{"The deadline must be non-negative."_el, values->value("timeout"_el)};
        }
        _timeout = el::TimeDelta::milliseconds(timeout);
        loop()->invoke([this]() -> void { begin(); });
    }

private:
    void begin() {
        el::io::printLine("Looking up server: "_el, _host.toString());
        // This deadline belongs to the application, independently of the lookup.
        loop()->invokeAfter(_timeout, [this]() -> void { onTimeout(); });
        const auto lookup = loop()->get<el::Network>().createHostLookup();
        lookup->events()
            .onResolved([host = _host](const el::List<el::IpAddress> &addresses) -> void {
                for (const auto &address : addresses) {
                    el::io::printLine("Server address for "_el, host.toString(), ": "_el, address);
                }
            })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                el::io::printLine("Lookup failed: "_el, error.title());
                _exitCode = el::ExitCode::failure();
            })
            .onFinal([this]() -> void {
                _finished = true;
                quit(_exitCode);
            });
        lookup->start(_host);
        el::io::printLine("Setup complete; waiting for the result."_el);
        if (_timeout.isZero()) {
            onTimeout();
        }
        // BUG: the only owning pointer is destroyed here, before result dispatch.
    }
    void onTimeout() {
        if (_finished) {
            return;
        }
        _finished = true;
        el::io::printLine("Deadline reached without a lookup result."_el);
        quit(el::ExitCode::failure());
    }

private:
    el::Host _host;                                  ///< Server selected on the command line.
    el::TimeDelta _timeout;                          ///< Independent application deadline.
    el::ExitCode _exitCode{el::ExitCode::success()}; ///< Result reported when the operation finishes.
    bool _finished{false};                           ///< Prevents a late deadline from changing a completed result.
};

}
