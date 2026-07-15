// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/event/EventRegistry.hpp>
#include <erbsland/unittest/UnitTest.hpp>

// notes:
// The event registry is a singleton managed by the `Application` framework.
// Therefore, we use `ApplicationTestScope` to get a fresh singleton for each test.

using namespace el::event;
using namespace el::text::literals;

TESTED_TARGETS(EventRegistry EventId EventBackendId)
class EventRegistryTest final : public el::UnitTest {
public:
    void testEventId() {
        REQUIRE_EQUAL(EventId{}.isValid(), false);
        REQUIRE_EQUAL(EventId{}.toRawValue(), 0);
    }

    void testBackendId() {
        REQUIRE_EQUAL(EventBackendId{}.isValid(), false);
        REQUIRE_EQUAL(EventBackendId{}.toRawValue(), 0);
    }

    void testInitialRegistry() {
        ApplicationTestScope appScope;
        const auto &eventRegistry = appScope.app().eventRegistry();
        REQUIRE_EQUAL(eventRegistry.isRegistered(id::NoEvent), true);
        REQUIRE_EQUAL(eventRegistry.isRegistered(id::QuitEvent), true);
        REQUIRE_EQUAL(eventRegistry.isRegistered(id::InvocationEvent), true);
        REQUIRE_EQUAL(eventRegistry.isRegistered(id::TimerEvent), true);
        REQUIRE_EQUAL(eventRegistry.isRegistered(id::NoBackend), true);
        REQUIRE_EQUAL(eventRegistry.isRegistered(id::SchedulerBackend), true);
    }

    void testCustomEventId() {
        ApplicationTestScope appScope;
        auto &eventRegistry = appScope.app().eventRegistry();
        const auto customIdA = eventRegistry.registerEvent("com.example.CustomEvent.A"_el, "Example"_el);
        REQUIRE_EQUAL(customIdA.isValid(), true);
        REQUIRE_GREATER_EQUAL(customIdA.toRawValue(), 0x10000U);
        REQUIRE_EQUAL(eventRegistry.isRegistered(customIdA), true);
        const auto customIdB = eventRegistry.registerEvent("com.example.CustomEvent.B"_el, "Example"_el);
        REQUIRE_EQUAL(customIdB.isValid(), true);
        REQUIRE_GREATER_EQUAL(customIdB.toRawValue(), 0x10000U);
        REQUIRE_EQUAL(eventRegistry.isRegistered(customIdB), true);
        REQUIRE_NOT_EQUAL(customIdA.toRawValue(), customIdB.toRawValue());

        const auto infoA = eventRegistry.getEventInfo(customIdA);
        REQUIRE_EQUAL(infoA.name(), "com.example.CustomEvent.A"_el);
        REQUIRE_EQUAL(infoA.description(), "Example"_el);
        const auto infoB = eventRegistry.getEventInfo(customIdB);
        REQUIRE_EQUAL(infoB.name(), "com.example.CustomEvent.B"_el);
        REQUIRE_EQUAL(infoB.description(), "Example"_el);
    }

    void testCustomBackendId() {
        ApplicationTestScope appScope;
        auto &eventRegistry = appScope.app().eventRegistry();
        const auto customIdA = eventRegistry.registerBackend("com.example.CustomBackend.A"_el, "Example"_el);
        REQUIRE_EQUAL(customIdA.isValid(), true);
        REQUIRE_GREATER_EQUAL(customIdA.toRawValue(), 0x10000U);
        REQUIRE_EQUAL(eventRegistry.isRegistered(customIdA), true);
        REQUIRE_EQUAL(eventRegistry.isBackendRegistered("com.example.CustomBackend.A"_el), true);
        REQUIRE_EQUAL(eventRegistry.getBackendId("com.example.CustomBackend.A"_el), customIdA);

        const auto infoA = eventRegistry.getBackendInfo(customIdA);
        REQUIRE_EQUAL(infoA.name(), "com.example.CustomBackend.A"_el);
        REQUIRE_EQUAL(infoA.description(), "Example"_el);
    }
};
