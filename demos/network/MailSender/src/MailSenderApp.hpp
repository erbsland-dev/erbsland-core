// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SenderSettings.hpp"

#include "../../MailCommon/src/MailApplication.hpp"
#include "../../MailCommon/src/PendingOutput.hpp"
#include "../../MailCommon/src/SmtpLineBuffer.hpp"
#include "../../MailCommon/src/SmtpReply.hpp"

#include <DemoTerminal.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ConnectionCloseContext.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/Events.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/NetworkErrorContext.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/TlsClientConnection.hpp>
#include <erbsland/TlsClientConnectOptions.hpp>

namespace demo {

using namespace el::text::literals;

/// Send one message through an authenticated TLS connection on the main event loop.
/// The application retains the connection and all protocol state through its final callback.
class MailSenderApp final : public MailApplication {
    enum class Stage { Greeting, Hello, Sender, Recipient, Data, Message, Quit, Complete };

public:
    using MailApplication::MailApplication;

protected: // implement el::Application
    void initialize() override {
        DemoTerminal::enable(*this);
        info().setApplicationName("TLS Mail Sender"_el);
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpDescription("Send one ASCII message to an SMTP server using implicit TLS (without AUTH)."_el);
        addCommonOptions(options);
        addTextOption(options, "smtp_host"_el, "Override the SMTP host name or IP address."_el);
        addTextOption(options, "smtp_port"_el, "Override the SMTP-over-TLS port."_el);
        addTextOption(options, "tls_profile"_el, "Select the TLS client profile label."_el);
        addTextOption(options, "sender_email"_el, "Override the envelope sender address."_el);
        addTextOption(options, "sender_name"_el, "Override the sender's display name."_el);
        addTextOption(options, "receiver_email"_el, "Override the recipient address."_el);
        addTextOption(options, "receiver_name"_el, "Override the recipient's display name."_el);
        addTextOption(options, "message"_el, "Override the plain ASCII message body."_el);
    }
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        setupLogging(values);
        const auto configuration = MailConfiguration{values, "mail_sender.elcl"_el};
        _settings = SenderSettings::load(configuration);
        _messageData = _settings.mail.wireData(); // Validate all mail input before opening a socket.
        cryptologyConfiguration().replaceTlsConfigurations(configuration.tlsProfiles());
        events()->invoke([this]() -> void { connectToServer(); });
    }
    void cleanup() noexcept override {
        if (_connection) {
            _connection->abort();
        }
    }

private:
    /// Select a trust profile and start one outgoing connection.
    void connectToServer() {
        _connection = events()->get<el::Network>().createTlsClientConnection();
        installHandlers();
        auto options = el::TlsClientConnectOptions{};
        options.setConfigurationLabel(_settings.tlsProfile);
        _mailLog->info("Connecting to "_el, _settings.endpoint.toString());
        _connection->connect(_settings.endpoint, options);
    }
    /// Observe authentication separately from TCP connectivity and application readiness.
    void installHandlers() {
        _connection->events()
            .onTransportConnected([this]() -> void { _mailLog->trace("TCP connected; authenticating the server."_el); })
            .onPeerHello([this]() -> void {
                _mailLog->trace("Server selected cipher "_el, _connection->cipherSuite()->toString());
            })
            .onPeerAuthenticated([this]() -> void {
                _mailLog->trace(
                    "Server certificate authenticated; path contains "_el,
                    _connection->peerCertificatePath().count(),
                    " certificates."_el);
            })
            .onHandshakeCompleted([this]() -> void {
                _mailLog->trace(
                    "TLS ready using profile "_el,
                    _connection->matchedConfigurationLabel(),
                    "; waiting for the SMTP greeting."_el);
            })
            .onData([this](el::ByteBlock data) -> void { receiveReplies(data); })
            .onWritable([this]() -> void { flushOutput(); })
            .onClosed([this](const el::ConnectionCloseContext &) -> void {
                _closedCleanly = true;
                _mailLog->trace("TLS close_notify exchange completed."_el);
            })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                _failed = true;
                _mailLog->error(
                    error.title(),
                    ": "_el,
                    error.description(),
                    " (phase "_el,
                    static_cast<int>(error.phase()),
                    ", reason "_el,
                    static_cast<int>(error.reason()),
                    ")"_el);
            })
            .onFinal([this]() -> void { finish(); });
    }
    /// Keep a back-pressured block until onWritable allows another attempt.
    void flushOutput() {
        while (!_output.isEmpty()) {
            const auto block = _output.nextBlock();
            const auto status = _connection->send(block);
            if (status.wouldBlock()) {
                return;
            }
            if (status.isClosed()) {
                fail("The connection closed with unsent SMTP data."_el);
                return;
            }
            _output.accepted(block.length());
        }
    }
    void sendCommand(const el::String &command) {
        _mailLog->trace("C: "_el, command);
        _output.append(el::join(command, "\r\n"_el));
        flushOutput();
    }
    void receiveReplies(const el::ByteBlock &data) {
        try {
            _lines.receive(data, [this](const el::String &line) -> bool {
                _mailLog->trace("S: "_el, line);
                if (const auto code = _reply.receive(line)) {
                    advanceConversation(*code);
                }
                return !_failed && _stage != Stage::Complete;
            });
        } catch (const el::ApplicationError &error) {
            fail(error.toString());
        }
    }
    void advanceConversation(const int code) {
        auto expected = 250;
        if (_stage == Stage::Greeting) {
            expected = 220;
        }
        if (_stage == Stage::Data) {
            expected = 354;
        }
        if (_stage == Stage::Quit) {
            expected = 221;
        }
        if (code != expected && !(_stage == Stage::Recipient && code == 251)) {
            fail(el::StringFormat{"SMTP rejected the exchange: expected {}, received {}."_el}.build(expected, code));
            return;
        }
        switch (_stage) {
        case Stage::Greeting:
            _stage = Stage::Hello;
            sendCommand("HELO localhost"_el);
            break;
        case Stage::Hello:
            _stage = Stage::Sender;
            sendCommand(el::StringFormat{"MAIL FROM:<{}>"_el}.build(_settings.mail.senderEmail));
            break;
        case Stage::Sender:
            _stage = Stage::Recipient;
            sendCommand(el::StringFormat{"RCPT TO:<{}>"_el}.build(_settings.mail.receiverEmail));
            break;
        case Stage::Recipient:
            _stage = Stage::Data;
            sendCommand("DATA"_el);
            break;
        case Stage::Data:
            _stage = Stage::Message;
            _mailLog->trace("C: [message data]"_el);
            _output.append(_messageData);
            flushOutput();
            break;
        case Stage::Message:
            _mailLog->info("Message accepted for "_el, _settings.mail.receiverEmail);
            _stage = Stage::Quit;
            sendCommand("QUIT"_el);
            break;
        case Stage::Quit:
            closeConnection();
            break;
        case Stage::Complete:
            break;
        }
    }
    /// Request TLS closure after the application protocol has finished.
    void closeConnection() {
        _stage = Stage::Complete;
        _connection->close();
    }
    void fail(const el::String &message) {
        _failed = true;
        _mailLog->error(message);
        _connection->abort();
    }
    /// Finalization is the single point that ends the application's event loop.
    void finish() {
        if (_failed || !_closedCleanly || _stage != Stage::Complete) {
            _mailLog->error("Mail exchange did not finish cleanly."_el);
            quit(el::ExitCode::failure());
            return;
        }
        _mailLog->info("Mail exchange completed."_el);
        quit();
    }

private:
    SenderSettings _settings;               ///< Validated configuration.
    el::TlsClientConnectionPtr _connection; ///< Retained until the event loop finishes.
    PendingOutput _output;                  ///< Blocks waiting for TLS send capacity.
    SmtpLineBuffer _lines;                  ///< Partial SMTP reply line.
    SmtpReply _reply;                       ///< Multiline reply state.
    el::String _messageData;                ///< Validated and escaped DATA payload.
    Stage _stage{Stage::Greeting};          ///< Next expected SMTP reply.
    bool _failed{false};                    ///< An operational failure was reported.
    bool _closedCleanly{false};             ///< Both TLS closure notifications were exchanged.
};

}
