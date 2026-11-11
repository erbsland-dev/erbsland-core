// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MailSession.hpp"
#include "ReceiverSettings.hpp"

#include "../../MailCommon/src/MailApplication.hpp"

#include <DemoTerminal.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/ConnectionQuota.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/NetworkErrorContext.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/TcpConnectionRequest.hpp>
#include <erbsland/TcpListener.hpp>
#include <erbsland/TcpListenerOptions.hpp>
#include <erbsland/TimeDelta.hpp>
#include <erbsland/TlsRole.hpp>
#include <erbsland/TlsServerAcceptOptions.hpp>

#include <map>
#include <memory>
#include <vector>

namespace demo {

using namespace el::text::literals;

/// Accept TLS mail sessions while retaining their state through asynchronous finalization.
class MailReceiverApp final : public MailApplication {
public:
    using MailApplication::MailApplication;

protected: // implement el::Application
    void initialize() override {
        DemoTerminal::enable(*this);
        info().setApplicationName("TLS Mail Receiver"_el);
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpDescription(
            "Receive one ASCII message per implicit-TLS connection and write it to the log."_el);
        addCommonOptions(options);
        addTextOption(options, "address"_el, "Override the local IP address."_el);
        addTextOption(options, "port"_el, "Override the local port; zero chooses an available port."_el);
        addTextOption(options, "tls_profile"_el, "Select the default TLS server profile."_el);
        addTextOption(options, "log_file"_el, "Append messages to this file as well as the console."_el);
        addTextOption(options, "timeout"_el, "Stop accepting after this positive number of seconds."_el);
        options->addOption({"--once"_el, "once"_el})
            .setType(el::OptionType::Flag)
            .setHelpTitle("Stop after one successful message and clean connection closure."_el);
    }
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        const auto configuration = MailConfiguration{values, "mail_receiver.elcl"_el};
        _settings = ReceiverSettings::load(configuration);
        setupLogging(values, _settings.logFile);
        cryptologyConfiguration().replaceTlsConfigurations(configuration.tlsProfiles());
        validateServerProfiles();
        _once = values->getFlag("once"_el);
        if (values->value("timeout"_el)) {
            _timeoutSeconds = values->getText("timeout"_el).toIntegerOrThrow<int>();
            if (_timeoutSeconds <= 0 || _timeoutSeconds > 86400) {
                throw el::ApplicationError{"Timeout must be between 1 and 86400 seconds."_el};
            }
        }
        loop()->invoke([this]() -> void { startListener(); });
    }
    void cleanup() noexcept override {
        if (_timeout) {
            _timeout->stop();
        }
        if (_shutdownDeadline) {
            _shutdownDeadline->stop();
        }
        if (_listener) {
            _listener->abort();
        }
        for (const auto &[id, session] : _sessions) {
            session->abort();
        }
    }

private:
    /// Resolve every configured server role before the listener becomes available.
    void validateServerProfiles() {
        auto labels = el::StringList{_settings.tlsProfile};
        for (const auto &mapping : _settings.mappings) {
            labels.append(mapping.configurationLabel());
        }
        for (const auto &profile : cryptologyConfiguration().resolveTlsConfigurations(labels, el::TlsRole::Server)) {
            if (!profile.configuration()->server() || !profile.configuration()->server()->identity()) {
                throw el::ApplicationError{
                    el::StringFormat{"TLS server profile '{}' has no identity."_el}.build(profile.requestedLabel())};
            }
        }
    }
    /// Bound accepted sockets and incomplete handshakes independently.
    void startListener() {
        _connectionQuota = el::ConnectionQuota::create(el::ItemCount{16});
        _handshakeQuota = el::ConnectionQuota::create(el::ItemCount{4});
        _listener = loop()->get<el::Network>().createTcpListener();
        _listener->events()
            .onListening(
                [this]() -> void { _mailLog->info("Listening on "_el, _listener->localEndpoint()->toString()); })
            .onConnection([this](el::TcpConnectionRequestPtr request) -> void { acceptConnection(std::move(request)); })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                _mailLog->error(
                    error.title(),
                    ": "_el,
                    error.description(),
                    " (phase "_el,
                    static_cast<int>(error.phase()),
                    ", reason "_el,
                    static_cast<int>(error.reason()),
                    ")"_el);
                _exitCode = el::ExitCode::failure();
                stopServer();
            })
            .onFinal([this]() -> void {
                _listenerFinished = true;
                finishIfStopped();
            });
        _listener->start(_settings.endpoint, el::TcpListenerOptions{}.setConnectionQuota(_connectionQuota));
        if (_timeoutSeconds > 0) {
            _timeout = loop()->createTimer([this]() -> void { stopServer(); });
            _timeout->startOnce(el::TimeDelta::seconds(_timeoutSeconds));
        }
    }
    /// Create and retain one TLS session for each accepted TCP request.
    void acceptConnection(el::TcpConnectionRequestPtr request) {
        if (_stopping || (_once && !_sessions.empty())) {
            request->reject();
            return;
        }
        const auto id = ++_nextSessionId;
        auto session = std::make_shared<MailSession>(loop(), _mailLog, [this, id](const bool delivered) -> void {
            // Defer destruction until the session's final callback has returned.
            loop()->invoke([this, id, delivered]() -> void { releaseSession(id, delivered); });
        });
        _sessions.emplace(id, session);
        auto options = el::TlsServerAcceptOptions{_handshakeQuota};
        options.setConfigurationLabel(_settings.tlsProfile).setConfigurationMappings(_settings.mappings);
        session->accept(std::move(request), std::move(options));
    }
    void releaseSession(const std::size_t id, const bool delivered) {
        _sessions.erase(id);
        if (_once && delivered) {
            stopServer();
        }
        finishIfStopped();
    }
    /// Stop admission first, then finish sessions while their event loop is still running.
    void stopServer() {
        if (_stopping) {
            return;
        }
        _stopping = true;
        _mailLog->info("Stopping the mail receiver."_el);
        _listener->close();
        for (const auto &[id, session] : _sessions) {
            session->stop();
        }
        _shutdownDeadline = loop()->createTimer([this]() -> void {
            for (const auto &[id, session] : _sessions) {
                session->abort();
            }
        });
        _shutdownDeadline->startOnce(el::TimeDelta::seconds(2));
        finishIfStopped();
    }
    void finishIfStopped() {
        if (_stopping && _listenerFinished && _sessions.empty()) {
            quit(_exitCode);
        }
    }

private:
    ReceiverSettings _settings;                                    ///< Listener and profile configuration.
    el::TcpListenerPtr _listener;                                  ///< Accepts transport requests on the main loop.
    el::ConnectionQuotaPtr _connectionQuota;                       ///< Bounds all accepted TCP sockets.
    el::ConnectionQuotaPtr _handshakeQuota;                        ///< Bounds incomplete TLS handshakes.
    std::map<std::size_t, std::shared_ptr<MailSession>> _sessions; ///< Owns sessions through onFinal.
    el::EventTimerPtr _timeout;                                    ///< Optional service lifetime.
    el::EventTimerPtr _shutdownDeadline;                           ///< Bounds draining during service shutdown.
    std::size_t _nextSessionId{0};                                 ///< Stable identifier for deferred removal.
    int _timeoutSeconds{0};                                        ///< Zero means no automatic stop.
    bool _once{false};                                             ///< Stop after one completed exchange.
    bool _stopping{false};                                         ///< Admission has ended.
    bool _listenerFinished{false};                                 ///< Listener has delivered onFinal.
    el::ExitCode _exitCode;                                        ///< Final service result.
};

}
