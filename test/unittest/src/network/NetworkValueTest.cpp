// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/network/Host.hpp>
#include <erbsland/network/HostEndpoint.hpp>
#include <erbsland/network/IpEndpoint.hpp>
#include <erbsland/network/Port.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <functional>

using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(Host HostName HostEndpoint IpEndpoint Port ScopeId)
class NetworkValueTest final : public el::UnitTest {
public:
    void testPorts() {
        REQUIRE(Port{}.isAutomatic());
        REQUIRE_EQUAL(Port{443}.toString(), "443"_el);
        REQUIRE_EQUAL(Port::fromStringOrThrow("0"_el).toRawValue(), uint16_t{0U});
        REQUIRE_EQUAL(Port::fromStringOrThrow("65535"_el).toRawValue(), uint16_t{65535U});
        REQUIRE_FALSE(Port::fromString(""_el).has_value());
        REQUIRE_FALSE(Port::fromString("-1"_el).has_value());
        REQUIRE_FALSE(Port::fromString("+1"_el).has_value());
        REQUIRE_FALSE(Port::fromString("65536"_el).has_value());
        REQUIRE_FALSE(Port::fromString("80 "_el).has_value());
        REQUIRE_FALSE(Port::fromString("80x"_el).has_value());
    }

    void testHostClassificationAndValidation() {
        const auto address = Host::fromStringOrThrow("2001:db8::1"_el);
        REQUIRE(address.isAddress());
        REQUIRE_FALSE(address.isName());
        const auto name = Host::fromStringOrThrow("printer.local"_el);
        REQUIRE(name.isName());
        REQUIRE_EQUAL(name.toString(), "printer.local"_el);
        REQUIRE_FALSE(HostName::fromString(""_el).has_value());
        REQUIRE_FALSE(HostName::fromString("bad\nname"_el).has_value());
        REQUIRE_FALSE(HostName::fromString("bad name"_el).has_value());
        REQUIRE_FALSE(Host::fromString("bad:address"_el).has_value());
        REQUIRE_FALSE(Host::fromString("fe80::1%4"_el).has_value());
    }

    void testEndpointsAndScopes() {
        const auto v4 = IpEndpoint::fromStringOrThrow("192.0.2.4:80"_el);
        REQUIRE_EQUAL(v4.toString(), "192.0.2.4:80"_el);
        const auto v6 = IpEndpoint::fromStringOrThrow("[fe80::1%7]:443"_el);
        REQUIRE_EQUAL(v6.toString(), "[fe80::1%7]:443"_el);
        REQUIRE_EQUAL(v6.scopeId().toRawValue(), uint32_t{7U});
        const auto host = HostEndpoint::fromStringOrThrow("example.test:53"_el);
        REQUIRE_EQUAL(host.toString(), "example.test:53"_el);
        REQUIRE_EQUAL(HostEndpoint::fromStringOrThrow("[2001:db8::1]:443"_el).toString(), "[2001:db8::1]:443"_el);
        REQUIRE_FALSE(IpEndpoint::fromString("2001:db8::1:443"_el).has_value());
        REQUIRE_FALSE(IpEndpoint::fromString("[192.0.2.1]:80"_el).has_value());
        REQUIRE_FALSE(IpEndpoint::fromString("[fe80::1%name]:80"_el).has_value());
        REQUIRE_FALSE(IpEndpoint::fromString("[fe80::1%0]:80"_el).has_value());
        REQUIRE_FALSE(IpEndpoint::fromString("[fe80::1%4294967296]:80"_el).has_value());
        REQUIRE_FALSE(IpEndpoint::fromString("[fe80::1%7%8]:80"_el).has_value());
        REQUIRE_FALSE(IpEndpoint::fromString("[fe80::1%7]80"_el).has_value());
        REQUIRE_FALSE(IpEndpoint::fromString("[fe80::1%7]:80]"_el).has_value());
        REQUIRE_FALSE(IpEndpoint::fromString("[fe80::1%7]:"_el).has_value());
        REQUIRE_FALSE(HostEndpoint::fromString("example.test"_el).has_value());
        REQUIRE_FALSE(HostEndpoint::fromString("[example.test]:443"_el).has_value());
        REQUIRE_FALSE(HostEndpoint::fromString("example.test:+443"_el).has_value());
    }

    void testValueHashing() {
        const auto firstAddress = IpAddress::fromStringOrThrow("2001:db8::1"_el);
        const auto secondAddress = IpAddress::fromStringOrThrow("2001:0db8:0::1"_el);
        REQUIRE_EQUAL(std::hash<IpAddress>{}(firstAddress), std::hash<IpAddress>{}(secondAddress));

        const auto firstEndpoint = IpEndpoint::fromStringOrThrow("[fe80::1%4]:443"_el);
        const auto secondEndpoint = IpEndpoint::fromStringOrThrow("[fe80:0::1%4]:443"_el);
        REQUIRE_EQUAL(std::hash<IpEndpoint>{}(firstEndpoint), std::hash<IpEndpoint>{}(secondEndpoint));
    }
};
