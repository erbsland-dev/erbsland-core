// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/network/HostName.hpp>
#include <erbsland/network/impl/HostResolver.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(HostResolver createHostResolver)
class HostResolverTest final : public el::UnitTest {
public:
    void testNativeResolverConvertsNumericIpv4() {
        const auto resolver = el::network::impl::createHostResolver();
        REQUIRE_NOT_EQUAL(resolver, nullptr);

        const auto addresses = resolver->resolve(HostName::fromStringOrThrow("127.0.0.1"_el));

        REQUIRE_FALSE(addresses.isEmpty());
        REQUIRE(addresses.contains(IpAddress::loopbackV4()));
        for (const auto &address : addresses) {
            REQUIRE(address.isV4() || address.isV6());
        }
    }
};
