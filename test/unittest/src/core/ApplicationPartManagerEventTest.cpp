// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ApplicationPartManager.hpp>
#include <erbsland/ApplicationPartManagerEventEditor.hpp>
#include <erbsland/CurrentLoop.hpp>
#include <erbsland/EventEditor.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventSource.hpp>
#include <erbsland/EventSubscription.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <future>
#include <memory>
#include <utility>

using namespace el::core;
using namespace el::event;

TESTED_TARGETS(ApplicationPartManager ApplicationPartManagerEventEditor EventSource EventEditor)
class ApplicationPartManagerEventTest final : public el::UnitTest {
public:
    void testStableEditorAndRetainedSource() {
        const auto loop = EventLoop::create();
        auto manager = ApplicationPartManager::create(loop);
        const auto weakManager = std::weak_ptr<ApplicationPartManager>{manager};
        auto &typedEditor = manager->events();
        auto &genericEditor = static_cast<EventSource &>(*manager).events();
        const auto &constEditor = static_cast<const EventEditor &>(typedEditor);

        REQUIRE_EQUAL(&genericEditor, static_cast<EventEditor *>(&typedEditor));
        REQUIRE_EQUAL(&typedEditor, &manager->events());
        REQUIRE_EQUAL(manager->ownerLoop(), loop);
        REQUIRE_EQUAL(constEditor.target(), loop);
        REQUIRE_EQUAL(constEditor.source(), manager);

        auto retainedSource = genericEditor.source();
        REQUIRE_EQUAL(retainedSource->shared_from_this(), retainedSource);
        manager.reset();
        REQUIRE_FALSE(weakManager.expired());
        REQUIRE_EQUAL(&retainedSource->events(), &genericEditor);
        retainedSource.reset();
        REQUIRE(weakManager.expired());
    }

    void testRegistrationOutsideOwnerLoopPreservesDeliveryTarget() {
        const auto owner = EventLoop::create();
        const auto other = EventLoop::create();
        const auto manager = ApplicationPartManager::create(owner);
        auto firstCalls = 0;
        auto secondCalls = 0;
        const auto first = manager->events().addStateChanged([&](ApplicationPartManagerState) -> void {
            REQUIRE_EQUAL(currentLoop(), owner);
            ++firstCalls;
        });
        auto second = EventSubscription{};
        other->invoke([&]() -> void {
            auto &editor = manager->events();
            REQUIRE_EQUAL(editor.source(), manager);
            REQUIRE_EQUAL(editor.target(), owner);
            second = editor.addStateChanged([&](ApplicationPartManagerState) -> void {
                REQUIRE_EQUAL(currentLoop(), owner);
                ++secondCalls;
            });
        });
        REQUIRE(other->runUntilIdle() > 0);
        manager->prepare();
        REQUIRE_EQUAL(firstCalls, 0);
        REQUIRE_EQUAL(secondCalls, 0);
        REQUIRE(owner->runUntilIdle() > 0);
        REQUIRE_EQUAL(firstCalls, 1);
        REQUIRE_EQUAL(secondCalls, 1);

        second.cancel();
        manager->start();
        REQUIRE(owner->runUntilIdle() > 0);
        REQUIRE_EQUAL(firstCalls, 3);
        REQUIRE_EQUAL(secondCalls, 1);
        REQUIRE_EQUAL(manager->ownerLoop(), owner);
        manager->stop();
        REQUIRE(owner->runUntilIdle() > 0);
    }

    void testOwnedControlLoopDeliversNotifications() {
        const auto manager = ApplicationPartManager::create();
        const auto owner = manager->ownerLoop();
        REQUIRE(owner != nullptr);
        REQUIRE_EQUAL(manager->events().target(), owner);
        const auto result = std::make_shared<std::promise<EventTargetPtr>>();
        auto future = result->get_future();
        const auto subscription =
            manager->events().addStateChanged([result](const ApplicationPartManagerState state) -> void {
                if (state == ApplicationPartManagerState::Running) {
                    result->set_value(currentLoop());
                }
            });
        manager->prepare();
        manager->start();
        REQUIRE(future.wait_for(std::chrono::seconds{5}) == std::future_status::ready);
        REQUIRE_EQUAL(future.get(), owner);
        manager->stop();
        REQUIRE(manager->waitForStopped());
    }

    void testControlTargetAccessBeforeLifecycleWork() {
        auto manager = ApplicationPartManager::create();
        const auto weakManager = std::weak_ptr<ApplicationPartManager>{manager};
        auto source = manager->events().source();
        const auto target = manager->events().target();
        const auto result = std::make_shared<std::promise<bool>>();
        auto future = result->get_future();
        manager.reset();
        target->invoke([source = std::move(source), result]() mutable -> void {
            auto waitRejected = false;
            try {
                static_cast<void>(std::static_pointer_cast<ApplicationPartManager>(source)->waitForRunning());
            } catch (const el::err::LogicFailure &) {
                waitRejected = true;
            }
            source.reset();
            result->set_value(waitRejected);
        });
        REQUIRE(future.wait_for(std::chrono::seconds{5}) == std::future_status::ready);
        REQUIRE(future.get());
        REQUIRE(weakManager.expired());
    }

    void testApplicationManagerUsesMainLoop() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        const auto manager = app.partManager();
        REQUIRE_EQUAL(manager->ownerLoop(), app.loop());
        REQUIRE_EQUAL(manager->events().target(), app.loop());
        REQUIRE_EQUAL(manager->events().source(), manager);
    }
};
