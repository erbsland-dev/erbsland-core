// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../../MailReceiver/src/MailTransaction.hpp"
#include "../../src/MailMessage.hpp"
#include "../../src/PendingOutput.hpp"
#include "../../src/SmtpLineBuffer.hpp"
#include "../../src/SmtpReply.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringDecoder.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/StringEncoding.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(MailMessage MailTransaction PendingOutput SmtpLineBuffer SmtpReply)
class SmtpProtocolTest final : public el::UnitTest {
public:
    void testFragmentedAndMultilineReplies() {
        auto input = demo::SmtpLineBuffer{};
        auto reply = demo::SmtpReply{};
        auto codes = std::vector<int>{};
        auto receive = [&](const el::String &line) -> bool {
            if (const auto code = reply.receive(line)) {
                codes.push_back(*code);
            }
            return true;
        };
        for (const auto &part : {"220-first\r"_el, "\n220-second\r\n22"_el, "0 ready\r\n250 OK\r\n"_el}) {
            input.receive(el::StringEncoder{el::String{part}}.encode(el::StringEncoding::Utf8), receive);
        }
        REQUIRE_EQUAL(codes.size(), 2U);
        REQUIRE_EQUAL(codes[0], 220);
        REQUIRE_EQUAL(codes[1], 250);
        REQUIRE_THROWS_AS(el::ApplicationError, reply.receive("25x bad"_el));
        REQUIRE_FALSE(reply.receive("250-first"_el));
        REQUIRE_THROWS_AS(el::ApplicationError, reply.receive("220 wrong code"_el));
    }
    void testInvalidAndOversizedLines() {
        auto input = demo::SmtpLineBuffer{};
        auto receive = [](const el::String &) -> bool { return true; };
        auto bytes = [](const el::String &text) -> el::ByteBlock {
            return el::StringEncoder{text}.encode(el::StringEncoding::Utf8);
        };
        REQUIRE_THROWS_AS(el::ApplicationError, input.receive(bytes("bare\n"_el), receive));
        input = {};
        REQUIRE_THROWS_AS(el::ApplicationError, input.receive(bytes("bad\rX"_el), receive));
        input = {};
        REQUIRE_THROWS_AS(el::ApplicationError, input.receive(bytes("café\r\n"_el), receive));
        input = {};
        REQUIRE_THROWS_AS(
            el::ApplicationError, input.receive(bytes(el::String::fromCharacter('x', el::CpCount{1000})), receive));
    }
    void testDotEscapingAndTransactionOrder() {
        const auto mail = demo::MailMessage{
            "alex@example.test"_el, "Alex"_el, "sam@example.test"_el, "Sam"_el, "First line\n.dot\n.\nlast"_el};
        const auto wire = mail.wireData();
        REQUIRE(wire.contains("\r\n..dot\r\n..\r\nlast\r\n.\r\n"_el));
        auto transaction = demo::MailTransaction{};
        REQUIRE(transaction.receive("DATA"_el).startsWith("503"_el));
        REQUIRE(transaction.receive("hElO localhost"_el).startsWith("250"_el));
        REQUIRE(transaction.receive("MAIL FROM:<alex@example.test>"_el).startsWith("250"_el));
        REQUIRE(transaction.receive("RCPT TO:<sam@example.test>"_el).startsWith("250"_el));
        REQUIRE(transaction.receive("DATA"_el).startsWith("354"_el));
        auto input = demo::SmtpLineBuffer{};
        const auto data = el::StringEncoder{wire}.encode(el::StringEncoding::Utf8);
        for (auto index = el::ByteIndex::zero(); index < data.endIndex(); ++index) {
            input.receive(data.slice(index, el::ByteCount::one()), [&](const el::String &line) -> bool {
                transaction.receive(line);
                return true;
            });
        }
        REQUIRE(transaction.isComplete());
        REQUIRE(transaction.message().contains("\r\n.dot\r\n.\r\nlast\r\n"_el));
        REQUIRE(transaction.receive("MAIL FROM:<again@example.test>"_el).startsWith("503"_el));
        REQUIRE(transaction.receive("QUIT"_el).startsWith("221"_el));
        REQUIRE(transaction.isClosing());
    }
    void testMessageInputValidation() {
        auto mail = demo::MailMessage{"alex@example.test"_el, "Alex"_el, "sam@example.test"_el, "Sam"_el, "hello"_el};
        mail.senderEmail = "alex@example.test\r\nRCPT TO:<other@example.test>"_el;
        REQUIRE_THROWS_AS(el::ApplicationError, mail.wireData());
        mail.senderEmail = "alex@example.test"_el;
        mail.senderName = "Alex\nInjected: value"_el;
        REQUIRE_THROWS_AS(el::ApplicationError, mail.wireData());
        mail.senderName = "Alex"_el;
        mail.message = "Grüezi"_el;
        REQUIRE_THROWS_AS(el::ApplicationError, mail.wireData());
        mail.message = el::String::fromCharacter('x', el::CpCount{999});
        REQUIRE_THROWS_AS(el::ApplicationError, mail.wireData());
        auto body = el::StringEditor{};
        for (int i = 0; i < 70; ++i) {
            body.append(el::String::fromCharacter('x', el::CpCount{998})).append("\n"_el);
        }
        mail.message = body;
        REQUIRE_THROWS_AS(el::ApplicationError, mail.wireData());
    }
    void testRetainedOutputAcrossWritableRetries() {
        auto output = demo::PendingOutput{};
        const auto content = el::String::fromCharacter('a', el::CpCount{20 * 1024});
        output.append(content);
        const auto first = output.nextBlock();
        REQUIRE_EQUAL(first.length(), el::ByteCount{16 * 1024});
        // A WouldBlock result leaves the queue untouched. Repeated attempts see the same block.
        REQUIRE_EQUAL(output.nextBlock(), first);
        output.accepted(first.length());
        const auto second = output.nextBlock();
        REQUIRE_EQUAL(second.length(), el::ByteCount{4 * 1024});
        output.accepted(second.length());
        REQUIRE(output.isEmpty());
        REQUIRE_THROWS_AS(el::ApplicationError, output.append(el::String::fromCharacter('x', el::CpCount{129 * 1024})));
    }
};
