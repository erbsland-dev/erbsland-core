// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LookupController.hpp"

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

/// Observe an ownership cycle after a real lookup, then explicitly dismantle it.
class LookupOwnershipCycleApp final : public el::Application {
public:
    using Application::Application;

public: // diagnostics
    /// Show that both normal and exceptional cleanup release the controller.
    void printCleanupResult() const {
        el::io::printLine("Controller released after cleanup: "_el, _controller.expired());
    }

protected: // implement Application
    void initialize() override { info().setApplicationName("Lookup Ownership Cycle"_el); }
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
    void cleanup() noexcept override {
        if (const auto controller = _controller.lock(); controller != nullptr) {
            controller->releaseSource();
        }
    }

private:
    void begin() {
        loop()->invokeAfter(_timeout, [this]() -> void { onTimeout(); });
        auto controller = std::make_shared<LookupController>();
        _controller = controller;
        controller->start(loop(), _host, [this](const el::ExitCode code) -> void {
            // Inspect after the source's final handler has returned.
            loop()->invoke([this, code]() -> void { inspectAndRelease(code); });
        });
        controller.reset();
        el::io::printLine("External controller owner released."_el);
        if (_timeout.isZero()) {
            onTimeout();
        }
    }
    void inspectAndRelease(const el::ExitCode code) {
        if (_finished) {
            return;
        }
        _finished = true;
        el::io::printLine("Controller still alive after completion: "_el, !_controller.expired());
        if (const auto controller = _controller.lock(); controller != nullptr) {
            controller->releaseSource(); // Break controller -> source -> callback -> controller.
        }
        el::io::printLine("Controller released after breaking the cycle: "_el, _controller.expired());
        quit(code);
    }
    void onTimeout() {
        if (_finished) {
            return;
        }
        _finished = true;
        cleanup(); // Also break the cycle if the expected completion never arrives.
        el::io::printLine("Deadline reached; controller released: "_el, _controller.expired());
        quit(el::ExitCode::failure());
    }

private:
    std::weak_ptr<LookupController> _controller; ///< Observes the cycle without adding another owning edge.
    el::Host _host;                              ///< Server selected on the command line.
    el::TimeDelta _timeout;                      ///< Independent application deadline.
    bool _finished{false};                       ///< Prevents a late deadline from changing a completed result.
};

}
