// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/BindEvent.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/List.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/String.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <exception>
#include <functional>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using namespace el::event;
using namespace el::text::literals;

TESTED_TARGETS(bindEvent BoundEvent EventMethodHandler)
class BindEventTest final : public el::UnitTest {
    struct Recipient {
        int value{0};
        void add(int amount) { value += amount; }
        void report(int &result) const noexcept { result = value; }
        auto result() -> int { return value; }
    };
    struct DerivedRecipient final : Recipient {};
    struct ThrowingCopy {
        ThrowingCopy() = default;
        ThrowingCopy(const ThrowingCopy &) { throw el::err::RuntimeError{"copy"}; }
        ThrowingCopy(ThrowingCopy &&) = default;
        void operator()() {}
    };
    class ThrowingEventTarget final : public EventTarget {
    public:
        void post(Event) override { throw el::err::RuntimeError{"post"}; }
        void invoke(EventCallback) override { throw el::err::RuntimeError{"submit"}; }
        void invokeAfter(el::time::TimeDelta, EventCallback) override { throw el::err::RuntimeError{"submit"}; }

    private:
        auto getBackend(EventBackendId) -> EventBackend & override { throw el::err::RuntimeError{"backend"}; }
    };

    static void increment(int &value) { ++value; }
    void drain(const EventLoopPtr &loop) {
        (void)loop->runUntilIdle();
        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
    }

public:
    void testExplicitBindingAndFifo() {
        const auto loop = EventLoop::create();
        auto values = std::vector<int>{};
        const auto callback = bindEvent(loop, [&values](int value) -> void { values.push_back(value); });
        REQUIRE_EQUAL(loop->queuedEventCount(), std::size_t{0});
        callback(1);
        callback(2);
        callback(3);
        REQUIRE(values.empty());
        drain(loop);
        REQUIRE_EQUAL(values, std::vector<int>({1, 2, 3}));
    }

    void testCurrentLoopCaptureAndSameLoopDeferral() {
        const auto receiver = EventLoop::create();
        const auto sender = EventLoop::create();
        auto recipient = std::make_shared<Recipient>();
        auto callback = std::function<void(int)>{};
        auto method = std::function<void(int)>{};
        auto values = std::vector<int>{};
        receiver->invoke([&]() -> void {
            callback = bindEvent([&](int value) -> void {
                REQUIRE_EQUAL(currentLoop(), receiver);
                values.push_back(value);
            });
            method = bindEvent(recipient, &Recipient::add);
            callback(1);
            REQUIRE(values.empty());
        });
        drain(receiver);
        sender->invoke([&]() -> void {
            callback(2);
            method(4);
            REQUIRE_EQUAL(values.size(), std::size_t{1});
            REQUIRE_EQUAL(recipient->value, 0);
        });
        drain(sender);
        drain(receiver);
        REQUIRE_EQUAL(values, std::vector<int>({1, 2}));
        REQUIRE_EQUAL(recipient->value, 4);
    }

    void testOrdinaryThreadAndConcurrentProducers() {
        const auto loop = EventLoop::create();
        const auto receiverThread = std::this_thread::get_id();
        auto values = std::vector<int>(80, 0);
        const auto callback = bindEvent(loop, [&](int value) -> void {
            REQUIRE_EQUAL(std::this_thread::get_id(), receiverThread);
            REQUIRE_EQUAL(currentLoop(), loop);
            ++values.at(static_cast<std::size_t>(value));
        });
        auto threads = std::vector<std::jthread>{};
        auto errors = std::vector<std::exception_ptr>(4);
        for (int producer = 0; producer < 4; ++producer) {
            threads.emplace_back([&, producer]() -> void {
                try {
                    const auto copy = callback;
                    for (int item = 0; item < 20; ++item) {
                        if (item % 2 == 0) {
                            callback(producer * 20 + item);
                        } else {
                            copy(producer * 20 + item);
                        }
                    }
                } catch (...) {
                    errors[static_cast<std::size_t>(producer)] = std::current_exception();
                }
            });
        }
        threads.clear();
        for (const auto &error : errors) {
            REQUIRE(error == nullptr);
        }
        drain(loop);
        REQUIRE_EQUAL(values, std::vector<int>(80, 1));
    }

    void testMoveOnlyHandlerSharesStateAcrossCopies() {
        const auto loop = EventLoop::create();
        auto values = std::vector<int>{};
        auto callback = bindEvent(
            loop, [count = std::make_unique<int>(0), &values]() mutable -> void { values.push_back(++*count); });
        static_assert(std::is_copy_constructible_v<decltype(callback)>);
        const auto copy = callback;
        const auto erased = std::function<void()>{callback};
        const auto erasedCopy = erased;
        callback();
        copy();
        erased();
        erasedCopy();
        drain(loop);
        REQUIRE_EQUAL(values, std::vector<int>({1, 2, 3, 4}));
    }

    void testArgumentStorageAndFunctionForms() {
        const auto loop = EventLoop::create();
        auto values = std::vector<int>{};
        const auto generic = bindEvent(loop, [&values](auto value) -> void { values.push_back(value); });
        int original = 7;
        generic(original);
        original = 9;
        auto payload = std::make_unique<int>(11);
        const auto movable = bindEvent(
            loop, [&](std::unique_ptr<int> value, const int &other) -> void { values.push_back(*value + other); });
        movable(std::move(payload), original);
        REQUIRE(payload == nullptr);
        original = 20;
        const auto rvalue =
            bindEvent(loop, [&values](std::unique_ptr<int> &&value) -> void { values.push_back(*value); });
        rvalue(std::make_unique<int>(13));
        const auto function = bindEvent(loop, &increment);
        const auto functionReference = bindEvent(loop, increment);
        function(std::ref(original));
        functionReference(std::ref(original));
        const auto erasedHandler = std::function<void(int)>{[&values](int value) -> void { values.push_back(value); }};
        bindEvent(loop, erasedHandler)(15);
        drain(loop);
        REQUIRE_EQUAL(original, 22);
        REQUIRE_EQUAL(values, std::vector<int>({7, 20, 13, 15}));
    }

    void testCompletionSignatures() {
        using FileList = el::util::List<el::text::String>;
        const auto loop = EventLoop::create();
        auto scanned = false;
        auto packed = false;
        const auto error = std::make_exception_ptr(el::err::RuntimeError{"worker"});
        const auto scanDone = std::function<void(FileList, std::exception_ptr)>{
            bindEvent(loop, [&](FileList files, std::exception_ptr result) -> void {
                REQUIRE_EQUAL(files.first(), "file"_el);
                REQUIRE(result == error);
                scanned = true;
            })};
        const auto packDone =
            std::function<void(std::exception_ptr)>{bindEvent(loop, [&](std::exception_ptr result) -> void {
                REQUIRE(result == nullptr);
                packed = true;
            })};
        auto files = FileList{};
        files.append("file"_el);
        scanDone(std::move(files), error);
        packDone(nullptr);
        drain(loop);
        REQUIRE(scanned);
        REQUIRE(packed);
    }

    void testMemberFunctionsAndRetention() {
        const auto loop = EventLoop::create();
        auto recipient = std::make_shared<DerivedRecipient>();
        const auto weak = std::weak_ptr<DerivedRecipient>{recipient};
        auto result = 0;
        {
            const auto method = bindEvent(loop, recipient, &Recipient::add);
            const auto report = bindEvent(loop, std::shared_ptr<const Recipient>{recipient}, &Recipient::report);
            method(10);
            method(5);
            report(std::ref(result));
            recipient.reset();
            REQUIRE_FALSE(weak.expired());
        }
        REQUIRE_FALSE(weak.expired());
        drain(loop);
        REQUIRE_EQUAL(result, 15);
        REQUIRE(weak.expired());
    }

    void testInvocationConstraints() {
        const auto loop = EventLoop::create();
        const auto value = bindEvent(loop, [](int) -> void {});
        const auto reference = bindEvent(loop, [](int &) -> void {});
        const auto moveOnly = bindEvent(loop, [](std::unique_ptr<int>) -> void {});
        const auto nonVoid = bindEvent(loop, []() -> int { return 1; });
        const auto recipient = std::make_shared<Recipient>();
        const auto nonVoidMethod = bindEvent(loop, recipient, &Recipient::result);
        const auto wrongRecipient = bindEvent(loop, std::make_shared<int>(), &Recipient::add);
        static_assert(el::event::impl::EventBindableFunction<decltype(&increment)>);
        static_assert(el::event::impl::EventBindableFunction<decltype(increment) &>);
        static_assert(el::event::impl::EventBindableFunction<std::function<void()>>);
        auto moveOnlyHandler = [value = std::make_unique<int>(1)]() -> void { (void)value; };
        static_assert(el::event::impl::EventBindableFunction<decltype(moveOnlyHandler)>);
        static_assert(!el::event::impl::EventBindableFunction<decltype(moveOnlyHandler) &>);
        static_assert(!el::event::impl::EventBindableFunction<decltype(&Recipient::add)>);
        static_assert(std::is_invocable_v<decltype(value), int &>);
        static_assert(!std::is_invocable_v<decltype(value), std::unique_ptr<int>>);
        static_assert(!std::is_invocable_v<decltype(reference), int &>);
        static_assert(std::is_invocable_v<decltype(reference), std::reference_wrapper<int>>);
        static_assert(!std::is_invocable_v<decltype(moveOnly), std::unique_ptr<int> &>);
        static_assert(std::is_invocable_v<decltype(moveOnly), std::unique_ptr<int>>);
        static_assert(!std::is_invocable_v<decltype(nonVoid)>);
        static_assert(!std::is_invocable_v<decltype(nonVoidMethod)>);
        static_assert(!std::is_invocable_v<decltype(wrongRecipient), int>);
        static_assert(std::is_constructible_v<std::function<void(int)>, decltype(value)>);
        static_assert(!std::is_constructible_v<std::function<void()>, decltype(nonVoid)>);
        static_assert(!std::is_constructible_v<std::function<int(int)>, decltype(value)>);
    }

    void testInvalidBindingsAndMovedFromRecovery() {
        const auto loop = EventLoop::create();
        const auto recipient = std::make_shared<Recipient>();
        auto nullFunction = static_cast<void (*)(int &)>(nullptr);
        auto nullMethod = static_cast<void (Recipient::*)(int)>(nullptr);
        REQUIRE_THROWS_AS(el::err::ParameterFailure, bindEvent(EventTargetPtr{}, &increment));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, bindEvent(loop, nullFunction));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, bindEvent(loop, std::function<void()>{}));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, bindEvent(loop, std::shared_ptr<Recipient>{}, &Recipient::add));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, bindEvent(loop, recipient, nullMethod));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, bindEvent(EventTargetPtr{}, recipient, &Recipient::add));
        REQUIRE_THROWS_AS(el::err::LogicFailure, bindEvent(&increment));
        REQUIRE_THROWS_AS(el::err::LogicFailure, bindEvent(recipient, &Recipient::add));
        auto callback = bindEvent(loop, &increment);
        auto moved = std::move(callback);
        int result = 0;
        REQUIRE_THROWS_AS(el::err::LogicFailure, callback(std::ref(result)));
        moved(std::ref(result));
        callback = moved;
        callback(std::ref(result));
        moved = std::move(callback);
        REQUIRE_THROWS_AS(el::err::LogicFailure, callback(std::ref(result)));
        moved(std::ref(result));
        drain(loop);
        REQUIRE_EQUAL(result, 3);
    }

    void testExceptionBoundaries() {
        const auto loop = EventLoop::create();
        const auto throwing = ThrowingCopy{};
        REQUIRE_THROWS_AS(el::err::RuntimeError, bindEvent(loop, throwing));
        const auto storage = bindEvent(loop, [](ThrowingCopy) -> void {});
        REQUIRE_THROWS_AS(el::err::RuntimeError, storage(throwing));
        REQUIRE_EQUAL(loop->queuedEventCount(), std::size_t{0});
        const auto submission = bindEvent(std::make_shared<ThrowingEventTarget>(), []() -> void {});
        REQUIRE_THROWS_AS(el::err::RuntimeError, submission());
        const auto execution = bindEvent(loop, []() -> void { throw el::err::RuntimeError{"handler"}; });
        REQUIRE_NOTHROW(execution());
        (void)loop->runUntilIdle();
        REQUIRE(loop->hasError());
        REQUIRE_THROWS_AS(el::err::RuntimeError, std::rethrow_exception(loop->takeError()));
        REQUIRE_FALSE(loop->hasError());
    }

    void testRejectedPayloadCleanup() {
        const auto options =
            EventLoopOptions{}.setMaximumQueuedEvents(1).setQueueOverflowAction(EventQueueOverflowAction::Ignore);
        const auto loop = EventLoop::create(options);
        auto calls = 0;
        const auto callback = bindEvent(loop, [&](std::unique_ptr<std::shared_ptr<int>>) -> void { ++calls; });
        auto accepted = std::make_unique<std::shared_ptr<int>>(std::make_shared<int>(1));
        auto rejected = std::make_unique<std::shared_ptr<int>>(std::make_shared<int>(2));
        const auto acceptedWeak = std::weak_ptr<int>{*accepted};
        const auto rejectedWeak = std::weak_ptr<int>{*rejected};
        callback(std::move(accepted));
        callback(std::move(rejected));
        REQUIRE_FALSE(acceptedWeak.expired());
        REQUIRE(rejectedWeak.expired());
        drain(loop);
        REQUIRE(acceptedWeak.expired());
        REQUIRE_EQUAL(calls, 1);
        loop->quit();
        auto afterQuit = std::make_unique<std::shared_ptr<int>>(std::make_shared<int>(3));
        const auto afterQuitWeak = std::weak_ptr<int>{*afterQuit};
        callback(std::move(afterQuit));
        REQUIRE(afterQuitWeak.expired());
        drain(loop);
        REQUIRE_EQUAL(calls, 1);
    }

    void testPendingDeliveryDoesNotRetainTarget() {
        auto loop = EventLoop::create();
        const auto weakLoop = std::weak_ptr<EventLoop>{loop};
        auto lifetime = std::make_shared<int>(1);
        const auto weakLifetime = std::weak_ptr<int>{lifetime};
        auto calls = 0;
        {
            const auto callback = bindEvent(loop, [lifetime = std::move(lifetime), &calls]() -> void { ++calls; });
            callback();
        }
        REQUIRE_FALSE(weakLifetime.expired());
        loop.reset();
        REQUIRE(weakLoop.expired());
        REQUIRE(weakLifetime.expired());
        REQUIRE_EQUAL(calls, 0);
    }
};
