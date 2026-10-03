// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../MailCommon/src/MailMessage.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/ByteRange.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringSide.hpp>
#include <erbsland/StringTransformations.hpp>

namespace demo {

using namespace el::text::literals;

/// Minimal one-message SMTP transaction, independent of sockets and TLS.
class MailTransaction {
    enum class Stage { Hello, Sender, Recipient, Data, Body, Complete };

public: // protocol
    /// Process one complete command or DATA line.
    /// @param line ASCII input without CRLF.
    /// @return A reply including CRLF, or empty text while collecting DATA.
    /// @throws el::ApplicationError If message data exceeds the bounded storage limit.
    auto receive(const el::String &line) -> el::String {
        if (_stage == Stage::Body) {
            return receiveBody(line);
        }
        if (line.length() > el::ByteCount{510}) {
            return "500 Command too long\r\n"_el;
        }
        const auto command = el::toUppercase(line);
        if (command == "QUIT"_el) {
            _closing = true;
            return "221 Bye\r\n"_el;
        }
        if (command == "NOOP"_el) {
            return "250 OK\r\n"_el;
        }
        if (_stage == Stage::Complete) {
            return "503 One message per connection; send QUIT\r\n"_el;
        }
        if (command.startsWith("HELO "_el) || command.startsWith("EHLO "_el)) {
            resetEnvelope();
            _stage = Stage::Sender;
            return "250 localhost\r\n"_el;
        }
        if (command == "RSET"_el && _stage != Stage::Hello) {
            resetEnvelope();
            _stage = Stage::Sender;
            return "250 Reset\r\n"_el;
        }
        if (command.startsWith("MAIL FROM:<"_el) && _stage == Stage::Sender) {
            _sender = addressFrom(line, el::ByteIndex{11});
            _stage = Stage::Recipient;
            return "250 Sender accepted\r\n"_el;
        }
        if (command.startsWith("RCPT TO:<"_el) && _stage == Stage::Recipient) {
            _recipient = addressFrom(line, el::ByteIndex{9});
            _stage = Stage::Data;
            return "250 Recipient accepted\r\n"_el;
        }
        if (command == "DATA"_el && _stage == Stage::Data) {
            _stage = Stage::Body;
            return "354 End with a line containing a single dot\r\n"_el;
        }
        return "503 Unexpected command for this transaction\r\n"_el;
    }
    /// Test whether the message was received completely.
    [[nodiscard]] auto isComplete() const noexcept -> bool { return _stage == Stage::Complete; }
    /// Test whether the peer requested protocol closure.
    [[nodiscard]] auto isClosing() const noexcept -> bool { return _closing; }
    /// Get the accepted envelope sender.
    [[nodiscard]] auto sender() const -> const el::String & { return _sender; }
    /// Get the accepted envelope recipient.
    [[nodiscard]] auto recipient() const -> const el::String & { return _recipient; }
    /// Get the received message, including headers, without SMTP dot escaping.
    [[nodiscard]] auto message() const -> el::String { return _message; }

private:
    static auto addressFrom(const el::String &line, const el::ByteIndex start) -> el::String {
        if (!line.endsWith(">"_el)) {
            throw el::ApplicationError{"Expected a mailbox enclosed in angle brackets."_el};
        }
        const auto address = line.slice(el::ByteRange{start, el::ByteIndex::last(line.length())});
        MailMessage::validateAddress(address);
        return address;
    }
    auto receiveBody(el::String line) -> el::String {
        if (line == "."_el) {
            _stage = Stage::Complete;
            return "250 Message received\r\n"_el;
        }
        if (line.startsWith("."_el)) {
            line = line.slice(el::StringSide::Back, el::ByteIndex{1});
        }
        if (line.length() > el::ByteCount{998} ||
            _message.length() + line.length() + el::ByteCount{2} > el::ByteCount{64 * 1024}) {
            throw el::ApplicationError{"Message exceeds the demo's line or 64 KiB message limit."_el};
        }
        _message.append(line).append("\r\n"_el);
        return {};
    }
    void resetEnvelope() {
        _sender = {};
        _recipient = {};
        _message.clear();
    }

private:
    Stage _stage{Stage::Hello}; ///< Current SMTP transaction state.
    el::String _sender;         ///< Envelope sender.
    el::String _recipient;      ///< Envelope recipient.
    el::StringEditor _message;  ///< Bounded complete message data.
    bool _closing{false};       ///< QUIT was received.
};

}
