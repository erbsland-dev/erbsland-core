// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParseError.hpp>
#include <erbsland/network/IpAddress.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::network;
using namespace el::mem;
using namespace el::text::literals;
using namespace el::unit;

TESTED_TARGETS(IpAddress IpVersion)
class IpAddressTest final : public el::UnitTest {
public:
    void testIpv4RoundTrips() {
        for (const auto text : {"0.0.0.0"_el, "127.0.0.1"_el, "192.0.2.17"_el, "255.255.255.255"_el}) {
            const auto address = IpAddress::fromString(text);
            REQUIRE(address.has_value());
            REQUIRE(address->isV4());
            REQUIRE_EQUAL(address->toString(), text);
        }
    }

    void testIpv6Canonicalization() {
        struct Entry {
            el::text::String input;
            el::text::String expected;
        };
        for (
            const auto &[input, expected] : {
                Entry{"::"_el, "::"_el},
                Entry{"0:0:0:0:0:0:0:1"_el, "::1"_el},
                Entry{"2001:0DB8:0:0:1:0:0:1"_el, "2001:db8::1:0:0:1"_el},
                Entry{"2001:db8:0:1:1:1:1:1"_el, "2001:db8:0:1:1:1:1:1"_el},
                Entry{"1:0:0:2:0:0:3:4"_el, "1::2:0:0:3:4"_el},
                Entry{"1::"_el, "1::"_el},
                Entry{"1:2:3:4:5:6:192.0.2.1"_el, "1:2:3:4:5:6:c000:201"_el},
                Entry{"::192.0.2.1"_el, "::c000:201"_el},
                Entry{"::ffff:192.0.2.1"_el, "::ffff:192.0.2.1"_el},
            }) {
            const auto address = IpAddress::fromString(input);
            REQUIRE(address.has_value());
            REQUIRE(address->isV6());
            REQUIRE_EQUAL(address->toString(), expected);
        }
    }

    void testInvalidAddresses() {
        for (
            const auto text : {
                ""_el,
                "1.2.3"_el,
                "1.2.3.4.5"_el,
                "256.0.0.1"_el,
                "0000.0.0.0"_el,
                "+1.2.3.4"_el,
                "1.2.3.4 "_el,
                "1.2.3.4x"_el,
                "192.000.2.1"_el,
                "1..2.3"_el,
                ":"_el,
                ":::"_el,
                "1:2:3:4:5:6:7"_el,
                "1:2:3:4:5:6:7:8:9"_el,
                "12345::"_el,
                "1::2::3"_el,
                "1:2:3:4:5:6:7:8::"_el,
                "::1:2:3:4:5:6:7:8"_el,
                "2001:db8::192.0.2.1:5"_el,
                "::ffff:192.0.2.999"_el,
                "::ffff:192.000.2.1"_el,
                "fe80::1%3"_el,
            }) {
            REQUIRE_FALSE(IpAddress::fromString(text).has_value());
        }
    }

    void testSpecialAddresses() {
        REQUIRE(IpAddress::anyV4().isAny());
        REQUIRE(IpAddress::anyV6().isAny());
        REQUIRE(IpAddress::loopbackV4().isLoopback());
        REQUIRE(IpAddress::loopbackV6().isLoopback());
        REQUIRE_FALSE(IpAddress::loopbackV4().isAny());
        REQUIRE(IpAddress::fromStringOrThrow("127.9.8.7"_el).isLoopback());
        REQUIRE_THROWS_AS(el::err::ParseError, IpAddress::fromStringOrThrow("bad:address"_el));
    }

    void testByteStorage() {
        auto bytes = IpAddress::Bytes{};
        bytes.set(ByteIndex{0U}, Byte{192U});
        bytes.set(ByteIndex{1U}, Byte{0U});
        bytes.set(ByteIndex{2U}, Byte{2U});
        bytes.set(ByteIndex{3U}, Byte{17U});
        bytes.set(ByteIndex{15U}, Byte{0xffU});

        const auto address = IpAddress::fromBytes(IpVersion::V4, bytes);
        REQUIRE_EQUAL(address.toString(), "192.0.2.17"_el);
        REQUIRE_EQUAL(address, IpAddress::fromStringOrThrow("192.0.2.17"_el));
        REQUIRE_EQUAL(address.toHash(), IpAddress::fromStringOrThrow("192.0.2.17"_el).toHash());
        REQUIRE_EQUAL(address.bytes().get(ByteIndex{15U}), Byte{});
    }
};
