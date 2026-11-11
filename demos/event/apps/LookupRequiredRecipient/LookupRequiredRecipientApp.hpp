// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AddressReport.hpp"

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

/// Retain a required result recipient through its callback without forming an ownership cycle.
class LookupRequiredRecipientApp final : public el::Application {
public:
    using Application::Application;

public: // diagnostics
    /// Show that retiring the source also releases its callback's recipient.
    void printCleanupResult() const { el::io::printLine("Report released after cleanup: "_el, _report.expired()); }

protected: // implement Application
    void initialize() override { info().setApplicationName("Lookup with a Required Report"_el); }
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
    void cleanup() noexcept override { _lookup.reset(); }

private:
    void begin() {
        loop()->invokeAfter(_timeout, [this]() -> void { onTimeout(); });
        _lookup = loop()->get<el::Network>().createHostLookup();
        auto report = std::make_shared<AddressReport>(_host);
        _report = report; // A weak pointer lets the diagnostic observe destruction without preventing it.
        _lookup->events()
            .onResolved([report](const el::List<el::IpAddress> &addresses) -> void { report->onResolved(addresses); })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                el::io::printLine("Lookup failed: "_el, error.title());
                _exitCode = el::ExitCode::failure();
            })
            .onFinal([this]() -> void {
                _finished = true;
                _lookup.reset();
                quit(_exitCode);
            });
        _lookup->start(_host);
        report.reset(); // The callback has its own shared pointer to the report.
        el::io::printLine("Report retained by the callback: "_el, !_report.expired());
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
    el::HostLookupPtr _lookup;                       ///< Application-owned operation; its callback retains the report.
    std::weak_ptr<AddressReport> _report;            ///< Non-owning diagnostic only.
    el::Host _host;                                  ///< Server selected on the command line.
    el::TimeDelta _timeout;                          ///< Independent application deadline.
    el::ExitCode _exitCode{el::ExitCode::success()}; ///< Result reported when the operation finishes.
    bool _finished{false};                           ///< Prevents a late deadline from changing a completed result.
};

}
