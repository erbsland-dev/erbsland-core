// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../LogWriter.hpp"
#include "../SyslogLogWriterOptions.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../network/source/Connection_fwd.hpp"
#include "../../network/udp/UdpSocket_fwd.hpp"
#include "../../time/TimePoint.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>

namespace erbsland::log::impl {

/// An asynchronous RFC 5424 syslog writer using the manager's event loop.
/// @tested{LogConfigurationParserTest SyslogLogWriterTest}
class SyslogLogWriter final : public LogWriter, public std::enable_shared_from_this<SyslogLogWriter> {
private:
    /// Maximum payload accepted by the UDP socket abstraction.
    static constexpr auto cMaximumUdpMessageBytes = unit::ByteLength{65'507U};

public:
    /// Create a syslog writer with the given network settings.
    /// @param options The transport, endpoint, RFC 5424 fields, TLS label, and pending-data limit.
    explicit SyslogLogWriter(SyslogLogWriterOptions options = {});

public: // implements LogWriter
    ~SyslogLogWriter() override;
    void write(const LogEntryConstPtr &entry, const LogLineConstPtr &line) override;
    void flush() override;
    void close() noexcept override;

public: // accessors
    /// Get the number of messages dropped by the writer queue.
    [[nodiscard]] auto droppedMessages() const noexcept -> uint64_t { return _droppedMessages.load(); }

public: // tools
    /// Build one RFC 5424 message without transport framing.
    /// @param entry The immutable entry providing severity and timestamp fields.
    /// @param line The formatted log content used as the message body.
    /// @param options The RFC 5424 header-field and facility settings.
    /// @return A complete unframed RFC 5424 message.
    [[nodiscard]] static auto formatMessage(
        const LogEntry &entry, const LogLine &line, const SyslogLogWriterOptions &options) -> text::String;
    /// Apply RFC 6587 octet-counted framing.
    /// @param message The unframed RFC 5424 message.
    /// @return The byte-count prefix, separating space, and original message.
    [[nodiscard]] static auto frameMessage(const text::String &message) -> text::String;

private:
    /// Map a log level to an RFC 5424 severity.
    /// @param level The log severity to map.
    /// @return The corresponding RFC 5424 numeric severity.
    [[nodiscard]] static auto severity(LogLevel level) noexcept -> uint8_t;
    /// Validate an RFC 5424 printable ASCII header field.
    /// @param value The header-field text to validate.
    /// @param maximum The maximum permitted code-point count.
    /// @return `true` if the value is valid for an RFC 5424 header field.
    [[nodiscard]] static auto isHeaderField(const text::String &value, unit::CpLength maximum) noexcept -> bool;
    /// Add encoded data to the bounded pending queue.
    /// @param data The complete encoded transport payload to enqueue.
    void enqueue(mem::ByteBlock data);
    /// Create a transport when none is active and retry is due.
    void ensureTransport();
    /// Create the UDP transport.
    void createUdpTransport();
    /// Start a TCP connection.
    void createTcpTransport();
    /// Start a TLS connection using the configured label.
    void createTlsTransport();
    /// Send as much pending data as the active transport accepts.
    void drainPending();
    /// Reset a failed transport and schedule a reconnect.
    void transportFailed();
    /// Remove the first completely transmitted data block.
    void removeFront();

private:
    SyslogLogWriterOptions _options;          ///< Transport and RFC 5424 settings.
    std::deque<mem::ByteBlock> _pending;      ///< Encoded payloads awaiting transmission.
    unit::ByteLength _pendingBytes;           ///< Total bytes charged to the pending queue.
    network::UdpSocketPtr _udpSocket;         ///< Active UDP socket, when selected.
    network::ConnectionPtr _connection;       ///< Active or connecting TCP/TLS transport.
    bool _active{};                           ///< Whether the selected transport can currently send.
    bool _connecting{};                       ///< Whether a connection attempt is in progress.
    bool _closed{};                           ///< Whether the writer has been closed permanently.
    uint8_t _retryExponent{};                 ///< Current bounded reconnect-backoff exponent.
    time::TimePoint _nextRetry;               ///< Earliest time for the next connection attempt.
    std::atomic<uint64_t> _droppedMessages{}; ///< Messages rejected by the bounded pending queue.
};

}
