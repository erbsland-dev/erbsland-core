// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MailTransaction.hpp"

#include "../../MailCommon/src/PendingOutput.hpp"
#include "../../MailCommon/src/SmtpLineBuffer.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ConnectionCloseContext.hpp>
#include <erbsland/ConnectionState.hpp>
#include <erbsland/Events.hpp>
#include <erbsland/LogStream.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/NetworkErrorContext.hpp>
#include <erbsland/String.hpp>
#include <erbsland/TcpConnectionRequest.hpp>
#include <erbsland/TlsServerAcceptOptions.hpp>
#include <erbsland/TlsServerConnection.hpp>

#include <functional>
#include <memory>

namespace demo {

using namespace el::text::literals;

/// Own one accepted TLS connection and one SMTP transaction on the listener's event loop.
/// The receiver retains this session until a deferred notification following onFinal.
class MailSession : public std::enable_shared_from_this<MailSession> {
public:
    /// Create a session and its inactive TLS connection.
    /// @param events The owning event collection.
    /// @param log The shared diagnostic stream.
    /// @param finished Notification after finalization; true means a message and clean TLS closure.
    MailSession(const el::EventsPtr &events, el::LogStreamPtr log, std::function<void(bool)> finished) :
        _events{events},
        _connection{events->get<el::Network>().createTlsServerConnection()},
        _log{std::move(log)},
        _finished{std::move(finished)} {}

public: // lifetime
    /// Install callbacks before adopting the accepted transport.
    /// @param request Ownership of one pending TCP request.
    /// @param options Default/SNI profiles and shared handshake quota.
    void accept(el::TcpConnectionRequestPtr request, el::TlsServerAcceptOptions options) {
        installHandlers();
        _connection->accept(std::move(request), std::move(options));
    }
    /// End a completed transaction gracefully; discard incomplete work on shutdown.
    void stop() {
        if (_transaction.isComplete()) {
            _closing = true;
            flushOutput();
        } else {
            _connection->abort();
        }
    }
    /// Abort pending work during application cleanup.
    void abort() noexcept { _connection->abort(); }

private:
    void installHandlers() {
        _connection->events()
            .onTransportConnected([this]() -> void { _log->trace("Accepted TCP transport; waiting for TLS."_el); })
            .onClientHello([this]() -> void { inspectClientHello(); })
            .onHandshakeCompleted([this]() -> void {
                // Let the handshake checkpoint return before sending application bytes.
                _events->invoke([self = shared_from_this()]() -> void {
                    if (self->_connection->state() == el::ConnectionState::Active) {
                        self->beginConversation();
                    }
                });
            })
            .onData([this](el::ByteBlock data) -> void { receiveCommands(data); })
            .onWritable([this]() -> void { flushOutput(); })
            .onClosed([this](const el::ConnectionCloseContext &) -> void {
                _closedCleanly = true;
                _log->trace("TLS close_notify exchange completed."_el);
            })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                _log->error(
                    error.title(),
                    ": "_el,
                    error.description(),
                    " (phase "_el,
                    static_cast<int>(error.phase()),
                    ", reason "_el,
                    static_cast<int>(error.reason()),
                    ")"_el);
            })
            .onFinal([this]() -> void { _finished(_logged && _closedCleanly); });
    }
    /// Inspect the profile selected by the client's requested server name.
    void inspectClientHello() {
        if (const auto name = _connection->serverName()) {
            _log->trace("Client requested server name "_el, name->toString());
        }
        _log->trace(
            "Selected server profile "_el,
            *_connection->matchedConfigurationLabel(),
            " with cipher "_el,
            _connection->cipherSuite()->toString());
    }
    /// Start the application protocol only after the TLS handshake has completed.
    void beginConversation() {
        if (_connection->isPeerAuthenticated()) {
            _log->info(
                "Client certificate authenticated; path contains "_el,
                _connection->peerCertificatePath().count(),
                " certificates."_el);
        } else {
            _log->trace("TLS ready; client did not present an authenticated certificate."_el);
        }
        sendReply("220 localhost TLS mail receiver\r\n"_el);
    }
    /// Process bounded SMTP lines; invalid client input affects only this session.
    void receiveCommands(const el::ByteBlock &data) {
        if (_closing) {
            return;
        }
        try {
            _lines.receive(data, [this](const el::String &line) -> bool {
                const auto reply = _transaction.receive(line);
                if (_transaction.isComplete() && !_logged) {
                    logMessage();
                }
                if (_transaction.isClosing()) {
                    _closing = true;
                }
                if (!reply.isEmpty()) {
                    sendReply(reply);
                }
                return !_closing;
            });
        } catch (const el::ApplicationError &error) {
            _log->warn(error.toString());
            _closing = true;
            _connection->abort();
        }
    }
    void logMessage() {
        _log->info("Received mail from "_el, _transaction.sender(), " for "_el, _transaction.recipient());
        _log->info(_transaction.message().replacedAll("\r\n"_el, "\n"_el));
        _logged = true;
    }
    void sendReply(const el::String &reply) {
        _log->trace("S: "_el, reply.trimmed());
        _output.append(reply);
        flushOutput();
    }
    /// Drain accepted output before requesting TLS closure.
    void flushOutput() {
        while (!_output.isEmpty()) {
            const auto block = _output.nextBlock();
            const auto status = _connection->send(block);
            if (status.wouldBlock()) {
                _connection->pauseReceiving();
                return;
            }
            if (status.isClosed()) {
                _connection->abort();
                return;
            }
            _output.accepted(block.length());
        }
        if (_closing) {
            _connection->close();
        } else {
            _connection->resumeReceiving();
        }
    }

private:
    el::EventsPtr _events;                  ///< Owns the queue used to start application work after the checkpoint.
    el::TlsServerConnectionPtr _connection; ///< TLS source owned until finalization.
    el::LogStreamPtr _log;                  ///< Shared diagnostic stream.
    std::function<void(bool)> _finished;    ///< Defers removal through the application.
    MailTransaction _transaction;           ///< One bounded SMTP transaction.
    SmtpLineBuffer _lines;                  ///< Incomplete command or DATA line.
    PendingOutput _output;                  ///< Replies waiting for send capacity.
    bool _logged{false};                    ///< The complete message has been logged.
    bool _closing{false};                   ///< Close once all replies have been accepted.
    bool _closedCleanly{false};             ///< Peer exchanged TLS closure notifications.
};

}
