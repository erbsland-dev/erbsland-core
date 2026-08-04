// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/EventLoop.hpp>
#include <erbsland/network/host_lookup/HostLookup.hpp>
#include <erbsland/network/host_lookup/HostLookupEventEditor.hpp>
#include <erbsland/network/Network.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/time/TimeUnitTags.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <set>

using namespace el::event;
using namespace el::network;
using namespace el::text::literals;
using namespace el::time;

TAGS(LiveNetwork DNS)
TESTED_TARGETS(HostLookup Network)
SKIP_BY_DEFAULT()
class HostLookupLiveDnsTest final : public el::UnitTest {
public:
    SKIP_BY_DEFAULT()
    void testErbslandDev() { resolvePublicHost("erbsland.dev"_el); }

    SKIP_BY_DEFAULT()
    void testCoreErbslandDev() { resolvePublicHost("core.erbsland.dev"_el); }

private:
    void resolvePublicHost(const el::text::String &hostText) {
        const auto loop = EventLoop::create();
        auto lookup = HostLookupPtr{};
        auto result = el::util::List<IpAddress>{};
        auto resolved = false;
        auto failed = false;

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onResolved([&](const auto &addresses) -> void {
                    result = addresses;
                    resolved = true;
                })
                .onError([&](const NetworkErrorContext &) -> void { failed = true; });
            lookup->start(Host::fromStringOrThrow(hostText));
        });
        REQUIRE(loop->runOnce());

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
        while (!resolved && !failed && std::chrono::steady_clock::now() < deadline) {
            static_cast<void>(loop->runOnce(TimeDelta{Milliseconds{250}}));
        }
        if (!resolved && !failed) {
            lookup->cancel();
        }

        REQUIRE(resolved);
        REQUIRE_FALSE(failed);
        REQUIRE_FALSE(result.isEmpty());
        const auto state = lookup->state();
        REQUIRE_EQUAL(state, NetworkSourceState::Inactive);
        auto unique = std::set<IpAddress>{};
        for (const auto &address : result) {
            REQUIRE(address.isV4() || address.isV6());
            REQUIRE_FALSE(address.isAny());
            REQUIRE_FALSE(address.isLoopback());
            REQUIRE(unique.insert(address).second);
        }
    }
};
