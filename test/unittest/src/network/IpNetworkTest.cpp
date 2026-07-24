// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/network/IpNetwork.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(IpNetwork)
class IpNetworkTest final : public el::UnitTest {
public:
    void testIpv4NormalizationAndContainment() {
        const auto network = IpNetwork::fromStringOrThrow("192.0.2.129/24"_el);
        REQUIRE_EQUAL(network.toString(), "192.0.2.0/24"_el);
        REQUIRE_EQUAL(network.firstAddress().toString(), "192.0.2.0"_el);
        REQUIRE_EQUAL(network.lastAddress().toString(), "192.0.2.255"_el);
        REQUIRE(network.contains(IpAddress::fromStringOrThrow("192.0.2.42"_el)));
        REQUIRE_FALSE(network.contains(IpAddress::fromStringOrThrow("192.0.3.1"_el)));
    }

    void testIpv6NormalizationAndContainment() {
        const auto network = IpNetwork::fromStringOrThrow("2001:db8:abcd:1234::1/48"_el);
        REQUIRE_EQUAL(network.toString(), "2001:db8:abcd::/48"_el);
        REQUIRE(network.contains(IpAddress::fromStringOrThrow("2001:db8:abcd:ffff::1"_el)));
        REQUIRE_FALSE(network.contains(IpAddress::fromStringOrThrow("2001:db8:abce::1"_el)));
        REQUIRE(network.contains(IpNetwork::fromStringOrThrow("2001:db8:abcd:1::/64"_el)));
    }

    void testPrefixBoundaries() {
        REQUIRE_EQUAL(IpNetwork::fromStringOrThrow("203.0.113.7/32"_el).lastAddress().toString(), "203.0.113.7"_el);
        REQUIRE_EQUAL(IpNetwork::fromStringOrThrow("2001:db8::1/128"_el).lastAddress().toString(), "2001:db8::1"_el);
        REQUIRE_EQUAL(IpNetwork{}.lastAddress().toString(), "255.255.255.255"_el);
        REQUIRE_FALSE(IpNetwork::fromString("192.0.2.1/33"_el).has_value());
        REQUIRE_FALSE(IpNetwork::fromString("::1/129"_el).has_value());
        REQUIRE_FALSE(IpNetwork::fromString("192.0.2.1/"_el).has_value());
        REQUIRE_FALSE(IpNetwork::fromString("192.0.2.1/+24"_el).has_value());
        REQUIRE_FALSE(IpNetwork::fromString("192.0.2.1/24 "_el).has_value());
        REQUIRE_FALSE(IpNetwork::fromString("192.0.2.1/24/7"_el).has_value());
        REQUIRE_THROWS_AS(el::err::ParameterError, IpNetwork(IpAddress::anyV4(), 33U));
        REQUIRE_THROWS_AS(el::err::ParseError, IpNetwork::fromStringOrThrow("not-a-network"_el));
    }
};
