// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../src/PiWorker.hpp"

#include <erbsland/BindEvent.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/TimeAmounts.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <stdexcept>

TESTED_TARGETS(PiWorker)
class PiWorkerTest final : public el::UnitTest {
public:
    void testOnlyOneBatchIsProducedWithoutFurtherDemand() {
        auto advances = std::atomic<int>{0};
        auto first = std::promise<void>{};
        auto firstReady = first.get_future();
        auto finished = std::atomic<int>{0};
        auto error = std::exception_ptr{};
        int countAtDelivery = 0;
        auto worker = demo::PiWorker{[&]() -> std::optional<el::Char> {
            const auto count = ++advances;
            if (count == 1) {
                return el::Char{U'3'};
            }
            return el::Char{U'1'};
        }};
        worker.start(
            std::nullopt,
            [&](el::String digits) -> void {
                countAtDelivery = advances.load();
                if (digits.characterLength() != el::CpCount{32}) {
                    throw std::runtime_error{"Unexpected batch size"};
                }
                first.set_value();
            },
            [&](std::exception_ptr result) -> void {
                error = result;
                ++finished;
            });
        REQUIRE(firstReady.wait_for(std::chrono::seconds{1}) == std::future_status::ready);
        worker.requestStop();
        worker.join();
        REQUIRE_EQUAL(advances.load(), countAtDelivery);
        REQUIRE_EQUAL(finished.load(), 1);
        REQUIRE(error == nullptr);
    }
    void testCancellationWhileDeterminingADigit() {
        auto entered = std::promise<void>{};
        auto ready = entered.get_future();
        bool notified = false;
        auto batches = 0;
        auto completed = 0;
        auto worker = demo::PiWorker{[&]() -> std::optional<el::Char> {
            if (!notified) {
                notified = true;
                entered.set_value();
            }
            return std::nullopt;
        }};
        worker.start(
            std::nullopt,
            [&](el::String) -> void { ++batches; },
            [&](std::exception_ptr error) -> void {
                if (error) {
                    std::rethrow_exception(error);
                }
                ++completed;
            });
        REQUIRE(ready.wait_for(std::chrono::seconds{1}) == std::future_status::ready);
        worker.requestStop();
        worker.join();
        REQUIRE_EQUAL(batches, 0);
        REQUIRE_EQUAL(completed, 1);
    }
    void testBatchOrderAndExactFinalLimit() {
        const auto loop = el::EventLoop::create();
        auto history = el::StringEditor{};
        auto batches = 0;
        auto completed = false;
        auto worker = demo::PiWorker{};
        worker.start(
            el::CpCount{67},
            el::bindEvent(
                loop,
                [&](el::String digits) -> void {
                    REQUIRE(digits.characterLength() <= el::CpCount{32});
                    ++batches;
                    history.append(digits);
                    worker.requestNextBatch();
                }),
            el::bindEvent(loop, [&](std::exception_ptr error) -> void {
                REQUIRE(error == nullptr);
                completed = true;
            }));
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{1};
        while (!completed && std::chrono::steady_clock::now() < deadline) {
            (void)loop->runOnce(el::Milliseconds{10});
            REQUIRE_FALSE(loop->hasError());
        }
        REQUIRE(completed);
        worker.join();
        REQUIRE_EQUAL(history.characterLength(), el::CpCount{67});
        REQUIRE(batches >= 3);
        using namespace el::text::literals;
        REQUIRE_EQUAL(el::String{history}, "1415926535897932384626433832795028841971693993751058209749445923078"_el);
    }
};
