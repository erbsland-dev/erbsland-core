// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "../../src/InboxWatcher.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/TempDirectory.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(InboxWatcher InboxEvents)
class InboxWatcherTest final : public el::UnitTest {
public:
    void testReplaceStopAndReleaseDuringDelivery() {
        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        for (const auto name : {"a"_el, "b"_el, "c"_el}) {
            (directory->path() / name).content().writeTextOrThrow("order"_el);
        }
        const auto loop = el::EventLoop::create();
        auto seen = el::StringList{};
        std::weak_ptr<demo::InboxWatcher> weak;
        loop->invoke([&]() -> void {
            auto watcher = demo::InboxWatcher::create(directory->path());
            weak = watcher;
            auto &events = watcher->events();
            REQUIRE(events.source() == watcher);
            REQUIRE(events.target() == loop);
            events.onFileArrived([&](const el::Path &first) -> void {
                events.onFileArrived([&](const el::Path &second) -> void {
                    seen.append(second.name());
                    watcher->stop();
                    watcher.reset();
                    REQUIRE_FALSE(weak.expired());
                });
                seen.append(first.name());
            });
            watcher->start();
        });
        (void)loop->runUntilIdle();
        REQUIRE_EQUAL(seen.count(), el::ItemCount{2});
        REQUIRE(seen.get(el::ItemIndex{}) == "a"_el);
        REQUIRE(seen.get(el::ItemIndex{1}) == "b"_el);
        REQUIRE(weak.expired());
    }
};
