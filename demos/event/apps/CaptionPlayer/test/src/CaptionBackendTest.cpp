// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "../../src/CaptionBackend.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/EventBackendTarget.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(CaptionBackend)
class CaptionBackendTest final : public el::UnitTest {
    class Target final : public el::EventBackendTarget {
    public: // implement EventBackendTarget
        auto postFromBackend(el::Event event) -> bool override {
            if (reject) {
                return false;
            }
            events.push_back(std::move(event));
            return true;
        }
        void wakeFromBackend() noexcept override {}

    public:
        std::vector<el::Event> events;
        bool reject{};
    };

public:
    void testLatePollAndDispatch() {
        auto backend = demo::CaptionBackend{_protocol, _tracks};
        auto target = std::make_shared<Target>();
        auto seen = std::vector<demo::CueBoundary>{};
        auto finished = false;
        backend.start(
            _start,
            [&](const demo::CueBoundary &cue) -> void { seen.push_back(cue); },
            [&]() -> void { finished = true; });
        backend.attach(target, {});
        REQUIRE(backend.nextWakeTime() == _start + el::Milliseconds{10});
        backend.poll(_start);
        REQUIRE(target->events.empty());
        backend.poll(_start + el::Milliseconds{30});
        REQUIRE_EQUAL(target->events.size(), 3U);
        REQUIRE(backend.nextWakeTime() == _start + el::Milliseconds{40});
        backend.poll(_start + el::Seconds{1});
        REQUIRE_FALSE(backend.nextWakeTime().has_value());
        backend.poll(_start + el::Seconds{2});
        REQUIRE_EQUAL(target->events.size(), 6U);
        REQUIRE(seen.empty());
        REQUIRE_FALSE(backend.handleEvent(el::Event{el::EventRegistry::timer()}));
        for (const auto &event : target->events) {
            REQUIRE_FALSE(finished);
            REQUIRE(backend.handleEvent(event));
        }
        REQUIRE(finished);
        REQUIRE_EQUAL(seen.size(), 6U);
        REQUIRE(seen[2].cue == 0 && !seen[2].entering);
        REQUIRE(seen[3].track == 0 && seen[3].cue == 1 && !seen[3].entering);
        REQUIRE(seen[4].track == 1 && seen[4].entering);
    }

    void testQueueRejectionEndsPollWithFailure() {
        auto backend = demo::CaptionBackend{_protocol, _tracks};
        auto target = std::make_shared<Target>();
        target->reject = true;
        backend.start(_start, [](const demo::CueBoundary &) -> void {}, []() -> void {});
        backend.attach(target, {});
        REQUIRE_THROWS_AS(el::ApplicationError, backend.poll(_start + el::Seconds{1}));
        REQUIRE(target->events.empty());
    }

private:
    el::Application _app;
    demo::CaptionProtocol _protocol{demo::CaptionProtocol::registerWith(_app.eventRegistry())};
    el::TimePoint _start{el::TimePoint::now()};
    std::vector<demo::CaptionTrack> _tracks{
        {{},
            {{{}, el::Milliseconds{10}, el::Milliseconds{30}, {}},
                {{}, el::Milliseconds{20}, el::Milliseconds{40}, {}}}},
        {{}, {{{}, el::Milliseconds{40}, el::Milliseconds{50}, {}}}},
    };
};
