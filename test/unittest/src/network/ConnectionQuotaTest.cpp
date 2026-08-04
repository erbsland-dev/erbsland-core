// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/network/IpAddress.hpp>
#include <erbsland/network/IpEndpoint.hpp>
#include <erbsland/network/Port.hpp>
#include <erbsland/network/source/ConnectionQuota.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <optional>
#include <thread>
#include <vector>

using namespace el::network;
namespace unit = el::unit;

TESTED_TARGETS(ConnectionQuota ConnectionQuotaLease)
class ConnectionQuotaTest final : public el::UnitTest {
public:
    void testValidationAcquisitionMoveAndRelease() {
        REQUIRE_THROWS_AS(el::err::ParameterError, ConnectionQuota::create(unit::ItemCount{}));
        REQUIRE_THROWS_AS(el::err::ParameterError, ConnectionQuota::create(unit::ItemCount::infinite()));
        const auto quota = ConnectionQuota::create(unit::ItemCount{2U});
        const auto endpoint = IpEndpoint{IpAddress::loopbackV4(), Port{443U}};
        auto first = quota->tryAcquire(endpoint);
        auto second = quota->tryAcquire();
        REQUIRE(first.has_value());
        REQUIRE(second.has_value());
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{2U});
        REQUIRE_EQUAL(quota->available(), unit::ItemCount{});
        REQUIRE_FALSE(quota->tryAcquire().has_value());

        auto moved = std::move(*first);
        REQUIRE_FALSE(first->isAcquired());
        REQUIRE(moved.isAcquired());
        moved.release();
        moved.release();
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{1U});
        second.reset();
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{});
    }

    void testConcurrentBound() {
        constexpr auto cThreadCount = std::size_t{32U};
        const auto quota = ConnectionQuota::create(unit::ItemCount{8U});
        auto acquired = std::vector<std::optional<ConnectionQuotaLease>>(cThreadCount);
        auto threads = std::vector<std::thread>{};
        threads.reserve(cThreadCount);
        for (auto index = std::size_t{}; index < cThreadCount; ++index) {
            threads.emplace_back([&, index]() -> void { acquired[index] = quota->tryAcquire(); });
        }
        for (auto &thread : threads) {
            thread.join();
        }
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{8U});
        acquired.clear();
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{});
    }
};
