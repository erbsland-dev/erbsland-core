// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/network/impl/TlsAlertDescription.hpp>
#include <erbsland/network/impl/TlsHandshakeStream.hpp>
#include <erbsland/network/impl/TlsProtocolError.hpp>
#include <erbsland/network/impl/TlsRecordStream.hpp>
#include <erbsland/network/impl/TlsWireReader.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using el::mem::ByteBlock;
using el::mem::ByteBlockEditor;
using el::network::impl::TlsAlertDescription;
using el::network::impl::TlsHandshakeStream;
using el::network::impl::TlsProtocolError;
using el::network::impl::TlsRecordStream;
using el::network::impl::TlsWireReader;
using el::unit::ByteLength;

TESTED_TARGETS(TlsAlertDescription TlsProtocolError TlsWireReader TlsRecordStream TlsHandshakeStream)
class TlsWireCodecTest final : public el::UnitTest {
private:
    template <typename Function>
    void requireAlert(const TlsAlertDescription expected, Function function) {
        auto wasThrown = false;
        try {
            function();
        } catch (const TlsProtocolError &error) {
            wasThrown = true;
            REQUIRE_EQUAL(error.alert(), expected);
        }
        REQUIRE(wasThrown);
    }

public:
    void testStrictWireReader() {
        const auto bytes = ByteBlock({0x01U, 0x02U, 0x03U, 0x04U, 0x02U, 0xaaU, 0xbbU});
        auto reader = TlsWireReader{bytes.span()};
        REQUIRE_EQUAL(reader.readU8(), uint8_t{1U});
        REQUIRE_EQUAL(reader.readU16(), uint16_t{0x0203U});
        REQUIRE_EQUAL(reader.readU8(), uint8_t{4U});
        REQUIRE_EQUAL(ByteBlock::fromSpan(reader.readVector8()), ByteBlock({0xaaU, 0xbbU}));
        reader.requireEnd();

        const auto truncatedBytes = ByteBlock({0x00U});
        auto truncated = TlsWireReader{truncatedBytes.span()};
        requireAlert(TlsAlertDescription::DecodeError, [&] { static_cast<void>(truncated.readU16()); });
        const auto trailingBytes = ByteBlock({0x01U});
        auto trailing = TlsWireReader{trailingBytes.span()};
        requireAlert(TlsAlertDescription::DecodeError, [&] { trailing.requireEnd(); });
    }

    void testRecordFragmentationAndCoalescing() {
        const auto first = ByteBlock({0x16U, 0x03U, 0x01U, 0x00U, 0x03U, 0x01U, 0x02U, 0x03U});
        const auto second = ByteBlock({0x17U, 0x03U, 0x03U, 0x00U, 0x01U, 0xffU});
        auto combined = ByteBlockEditor{};
        combined.append(first).append(second);
        auto stream = TlsRecordStream{ByteLength{64U}};
        stream.append(combined.span().first(2U));
        REQUIRE_FALSE(stream.next().has_value());
        stream.append(combined.span().subspan(2U));
        REQUIRE_EQUAL(*stream.next(), first);
        REQUIRE_EQUAL(*stream.next(), second);
        REQUIRE_FALSE(stream.next().has_value());

        auto oversized = TlsRecordStream{ByteLength{64U}};
        oversized.append(ByteBlock({0x17U, 0x03U, 0x03U, 0x41U, 0x01U}).span());
        requireAlert(TlsAlertDescription::RecordOverflow, [&] { static_cast<void>(oversized.next()); });
        requireAlert(TlsAlertDescription::RecordOverflow, [&] {
            auto bounded = TlsRecordStream{ByteLength{5U}};
            bounded.append(ByteBlock(ByteLength{6U}).span());
        });
    }

    void testHandshakeFragmentationBoundsAndExactMessages() {
        const auto first = ByteBlock({0x01U, 0x00U, 0x00U, 0x03U, 0x11U, 0x22U, 0x33U});
        const auto second = ByteBlock({0x02U, 0x00U, 0x00U, 0x01U, 0x44U});
        auto combined = ByteBlockEditor{};
        combined.append(first).append(second);
        auto stream = TlsHandshakeStream{};
        for (const auto byte : combined.span()) {
            stream.append(ByteBlock({byte}).span());
        }
        REQUIRE_EQUAL(*stream.next(), first);
        REQUIRE_EQUAL(*stream.next(), second);
        REQUIRE_FALSE(stream.next().has_value());

        auto oversized = TlsHandshakeStream{};
        oversized.append(ByteBlock({0x0bU, 0x10U, 0x00U, 0x01U}).span());
        requireAlert(TlsAlertDescription::DecodeError, [&] { static_cast<void>(oversized.next()); });
    }
};
