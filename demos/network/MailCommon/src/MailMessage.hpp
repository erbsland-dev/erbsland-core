// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/DateTime.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringSplitter.hpp>
#include <erbsland/U8StringConstIterator.hpp>

#include <array>

namespace demo {

using namespace el::text::literals;

/// A single plain ASCII message, independent of its TLS transport.
struct MailMessage {
public:
    el::String senderEmail;   ///< Envelope sender address.
    el::String senderName;    ///< Display name in the From header.
    el::String receiverEmail; ///< Envelope recipient address.
    el::String receiverName;  ///< Display name in the To header.
    el::String message;       ///< Plain-text body, accepting LF or CRLF input.

public:                       // message encoding
    /// Validate input and create CRLF message data, including dot escaping and the terminator.
    /// @return Complete SMTP DATA text.
    /// @throws el::ApplicationError If addresses, names, or message text are unsupported.
    [[nodiscard]] auto wireData() const -> el::String {
        validateAddress(senderEmail);
        validateAddress(receiverEmail);
        validateName(senderName);
        validateName(receiverName);
        auto text = el::StringFormat{"From: \"{}\" <{}>\r\nTo: \"{}\" <{}>\r\n\r\n"_el}.build(
            senderName, senderEmail, receiverName, receiverEmail);
        text = el::join(dateHeader(), text);
        auto output = el::StringEditor{text};
        auto messageLength = text.length();
        auto lines = el::StringSplitter{message.replacedAll("\r\n"_el, "\n"_el), el::Char{'\n'}};
        while (!lines.isAtEnd()) {
            const auto line = lines.next();
            validateText(line);
            if (line.length() > el::ByteCount{998}) {
                throw el::ApplicationError{"Message lines must contain at most 998 ASCII characters."_el};
            }
            messageLength += line.length() + el::ByteCount{2};
            if (messageLength > el::ByteCount{64 * 1024}) {
                throw el::ApplicationError{"Message exceeds the 64 KiB demo limit."_el};
            }
            if (line.startsWith("."_el)) {
                output.append("."_el);
            }
            output.append(line).append("\r\n"_el);
        }
        output.append(".\r\n"_el);
        return output;
    }
    /// Validate a simple ASCII mailbox without quoted local parts or address literals.
    /// @param address The envelope address.
    /// @throws el::ApplicationError If the address is not supported by this demonstration.
    static void validateAddress(const el::String &address) {
        validateText(address);
        if (address.isEmpty() || address.length() > el::ByteCount{254} || address.count("@"_el) != el::ItemCount{1} ||
            address.startsWith("@"_el) || address.endsWith("@"_el)) {
            throw el::ApplicationError{"Use a simple ASCII mailbox such as reader@example.test."_el};
        }
        for (const auto character : address) {
            if (character <= ' ' || character == '<' || character == '>' || character == '"' || character == '\\') {
                throw el::ApplicationError{"Unsupported character in an email address."_el};
            }
        }
    }
    /// Validate printable ASCII body text, permitting horizontal tabs.
    /// @param text One line without a line ending.
    /// @throws el::ApplicationError If the text contains unsupported characters.
    static void validateText(const el::String &text) {
        for (const auto character : text) {
            if (character != '\t' && (character < ' ' || character > '~')) {
                throw el::ApplicationError{"This demo supports printable ASCII text only."_el};
            }
        }
    }

private:
    static auto dateHeader() -> el::String {
        static constexpr auto cMonths = std::array{
            "Jan"_el,
            "Feb"_el,
            "Mar"_el,
            "Apr"_el,
            "May"_el,
            "Jun"_el,
            "Jul"_el,
            "Aug"_el,
            "Sep"_el,
            "Oct"_el,
            "Nov"_el,
            "Dec"_el};
        const auto now = el::DateTime::now();
        return el::StringFormat{"Date: {} {} {} {:number:width=2,zero-fill}:{:number:width=2,zero-fill}:"
                                "{:number:width=2,zero-fill} +0000\r\n"_el}
            .build(
                now.day().toRawValue(),
                cMonths.at(static_cast<std::size_t>(now.month().toRawValue() - 1)),
                now.year().toRawValue(),
                now.hour().toRawValue(),
                now.minute().toRawValue(),
                now.second().toRawValue());
    }
    static void validateName(const el::String &name) {
        validateText(name);
        if (name.length() > el::ByteCount{100} || name.contains("\""_el) || name.contains("\\"_el)) {
            throw el::ApplicationError{"Display names must be short ASCII text without quotes or backslashes."_el};
        }
    }
};

}
