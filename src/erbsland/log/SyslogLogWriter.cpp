// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SyslogLogWriter.hpp"

#include "../err/ParameterError.hpp"
#include "../event/CurrentEvents.hpp"
#include "../event/Events.hpp"
#include "../network/Network.hpp"
#include "../network/source/Connection.hpp"
#include "../network/tcp/TcpConnection.hpp"
#include "../network/tls/TlsClientConnection.hpp"
#include "../network/tls/TlsClientConnectOptions.hpp"
#include "../network/udp/UdpSocket.hpp"
#include "../network/udp/UdpSocketOptions.hpp"
#include "../text/CharSet.hpp"
#include "../text/Literals.hpp"
#include "../text/StringEncoder.hpp"
#include "../text/StringFormat.hpp"
#include "../time/IsoTimeFormat.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::log {

using namespace text::literals;

SyslogLogWriter::SyslogLogWriter(SyslogLogWriterOptions options) : _options{std::move(options)} {
    if (_options.endpoint().port().isAutomatic()) {
        throw err::ParameterError{"A syslog destination requires a nonzero port."_el, "options"_el};
    }
    if (_options.transport() == SyslogTransport::Udp && !_options.endpoint().host().isAddress()) {
        throw err::ParameterError{"A UDP syslog destination must use a numeric IP address."_el, "options"_el};
    }
    if (!isHeaderField(_options.hostName(), unit::CpLength{255U}) ||
        !isHeaderField(_options.applicationName(), unit::CpLength{48U}) ||
        !isHeaderField(_options.processId(), unit::CpLength{128U}) ||
        !isHeaderField(_options.messageId(), unit::CpLength{32U})) {
        throw err::ParameterError{"Invalid RFC 5424 header field in syslog options."_el, "options"_el};
    }
}

SyslogLogWriter::~SyslogLogWriter() {
    if (_udpSocket != nullptr) {
        _udpSocket->abort();
    }
    if (_connection != nullptr) {
        _connection->abort();
    }
}

void SyslogLogWriter::write(const LogEntryConstPtr &entry, const LogLineConstPtr &line) {
    auto message = formatMessage(*entry, *line, _options);
    if (_options.transport() != SyslogTransport::Udp) {
        message = frameMessage(message);
    }
    enqueue(text::StringEncoder{message}.encode(text::StringEncoding::Utf8, text::StringBomMode::Reject));
    ensureTransport();
    drainPending();
}

void SyslogLogWriter::flush() {
    ensureTransport();
    drainPending();
}

void SyslogLogWriter::close() noexcept {
    _closed = true;
    if (_udpSocket != nullptr) {
        _udpSocket->close();
    }
    if (_connection != nullptr) {
        _connection->close();
    }
    _udpSocket.reset();
    _connection.reset();
    _pending.clear();
    _pendingBytes = {};
}

auto SyslogLogWriter::formatMessage(const LogEntry &entry, const LogLine &line, const SyslogLogWriterOptions &options)
    -> text::String {
    const auto priority = static_cast<unsigned>(options.facility()) * 8U + severity(entry.level());
    const auto timestamp = entry.timestamp().toIsoString(
        time::IsoTimeFormatFlags{
            time::IsoTimeFormat::Extended, time::IsoTimeFormat::TimePrefix, time::IsoTimeFormat::TimeShift});
    static const auto cMessageFormat = text::StringFormat{"<{}>1 {} {} {} {} {} - {}"_el};
    return cMessageFormat.build(
        priority,
        timestamp,
        options.hostName(),
        options.applicationName(),
        options.processId(),
        options.messageId(),
        line.text());
}

auto SyslogLogWriter::frameMessage(const text::String &message) -> text::String {
    static const auto cFrameFormat = text::StringFormat{"{} {}"_el};
    return cFrameFormat.build(message.length().toRawValue(), message);
}

auto SyslogLogWriter::severity(const LogLevel level) noexcept -> uint8_t {
    switch (level.toRawValue()) {
    case LogLevel::Trace:
        return 7U;
    case LogLevel::Information:
        return 6U;
    case LogLevel::Warning:
        return 4U;
    case LogLevel::Error:
        return 3U;
    case LogLevel::All:
        break;
    }
    return 5U;
}

auto SyslogLogWriter::isHeaderField(const text::String &value, const unit::CpLength maximum) noexcept -> bool {
    if (value.isEmpty() || value.characterLength() > maximum) {
        return false;
    }
    static const auto cHeaderCharacters = []() -> text::CharSet {
        auto result = text::CharSet::fromRange(U'!', U'~');
        result.remove(U'=');
        result.remove(U']');
        result.remove(U'"');
        return result;
    }();
    return value.containsOnly(cHeaderCharacters);
}

void SyslogLogWriter::enqueue(mem::ByteBlock data) {
    const auto length = data.length();
    const auto maximum = _options.maximumPendingBytes();
    if ((_options.transport() == SyslogTransport::Udp && data.length() > cMaximumUdpMessageBytes) || length > maximum ||
        _pendingBytes > maximum - length) {
        ++_droppedMessages;
        return;
    }
    _pendingBytes += length;
    _pending.emplace_back(std::move(data));
}

void SyslogLogWriter::ensureTransport() {
    if (_closed || _pending.empty() || _active || _connecting || time::TimePoint::now() < _nextRetry) {
        return;
    }
    switch (_options.transport().toRawValue()) {
    case SyslogTransport::Udp:
        createUdpTransport();
        break;
    case SyslogTransport::Tcp:
        createTcpTransport();
        break;
    case SyslogTransport::Tls:
        createTlsTransport();
        break;
    }
}

void SyslogLogWriter::createUdpTransport() {
    auto &network = event::currentEvents()->get<network::Network>();
    _udpSocket = network.createUdpSocket();
    _connecting = true;
    const auto weak = weak_from_this();
    _udpSocket->events()
        .onBound([weak]() -> void {
            if (const auto self = weak.lock()) {
                self->_connecting = false;
                self->_active = true;
                self->_retryExponent = 0U;
                self->drainPending();
            }
        })
        .onWritable([weak]() -> void {
            if (const auto self = weak.lock()) {
                self->drainPending();
            }
        })
        .onError([weak](const auto &) -> void {
            if (const auto self = weak.lock()) {
                self->transportFailed();
            }
        });
    auto socketOptions = network::UdpSocketOptions{};
    socketOptions.setSendQueueLimit(_options.maximumPendingBytes());
    _udpSocket->start(std::move(socketOptions));
}

void SyslogLogWriter::createTcpTransport() {
    auto &network = event::currentEvents()->get<network::Network>();
    auto connection = network.createTcpConnection();
    const auto weak = weak_from_this();
    connection->events()
        .onConnected([weak]() -> void {
            if (const auto self = weak.lock()) {
                self->_connecting = false;
                self->_active = true;
                self->_retryExponent = 0U;
                self->drainPending();
            }
        })
        .onWritable([weak]() -> void {
            if (const auto self = weak.lock()) {
                self->drainPending();
            }
        })
        .onError([weak](const auto &) -> void {
            if (const auto self = weak.lock()) {
                self->transportFailed();
            }
        })
        .onClosed([weak](const auto &) -> void {
            if (const auto self = weak.lock()) {
                self->transportFailed();
            }
        });
    _connection = connection;
    _connecting = true;
    connection->connect(_options.endpoint());
}

void SyslogLogWriter::createTlsTransport() {
    auto &network = event::currentEvents()->get<network::Network>();
    auto connection = network.createTlsClientConnection();
    const auto weak = weak_from_this();
    connection->events()
        .onHandshakeCompleted([weak]() -> void {
            if (const auto self = weak.lock()) {
                self->_connecting = false;
                self->_active = true;
                self->_retryExponent = 0U;
                self->drainPending();
            }
        })
        .onWritable([weak]() -> void {
            if (const auto self = weak.lock()) {
                self->drainPending();
            }
        })
        .onError([weak](const auto &) -> void {
            if (const auto self = weak.lock()) {
                self->transportFailed();
            }
        })
        .onClosed([weak](const auto &) -> void {
            if (const auto self = weak.lock()) {
                self->transportFailed();
            }
        });
    _connection = connection;
    _connecting = true;
    auto options = network::TlsClientConnectOptions{};
    options.setConfigurationLabel(_options.tlsConfigurationLabel());
    connection->connect(_options.endpoint(), std::move(options));
}

void SyslogLogWriter::drainPending() {
    if (!_active) {
        return;
    }
    while (!_pending.empty()) {
        auto status = network::NetworkSendStatus::Closed;
        if (_options.transport() == SyslogTransport::Udp) {
            const auto address = _options.endpoint().host().address();
            status = _udpSocket->send(
                network::IpEndpoint{*address, _options.endpoint().port(), _options.endpoint().scopeId()},
                _pending.front());
        } else {
            status = _connection->send(_pending.front());
        }
        if (status.isAccepted()) {
            removeFront();
            continue;
        }
        if (status.isClosed()) {
            transportFailed();
        }
        return;
    }
}

void SyslogLogWriter::transportFailed() {
    _active = false;
    _connecting = false;
    if (_udpSocket != nullptr) {
        _udpSocket->abort();
    }
    if (_connection != nullptr) {
        _connection->abort();
    }
    _udpSocket.reset();
    _connection.reset();
    const auto delaySeconds = 1U << std::min<uint8_t>(_retryExponent, 5U);
    _retryExponent = std::min<uint8_t>(static_cast<uint8_t>(_retryExponent + 1U), 5U);
    const auto delay = time::TimeDelta::seconds(delaySeconds);
    _nextRetry = time::TimePoint::inFuture(delay);
    const auto weak = weak_from_this();
    event::currentEvents()->invokeAfter(delay, [weak]() -> void {
        if (const auto self = weak.lock()) {
            self->ensureTransport();
            self->drainPending();
        }
    });
}

void SyslogLogWriter::removeFront() {
    _pendingBytes -= _pending.front().length();
    _pending.pop_front();
}

}
