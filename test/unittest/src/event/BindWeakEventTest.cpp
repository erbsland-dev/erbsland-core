// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/BindEvent.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

using namespace el::event;

TESTED_TARGETS(bindWeakEvent WeakEventMethodHandler)
class BindWeakEventTest final : public el::UnitTest {
    struct Recipient {
        int value{0};
        void add(int amount) { value += amount; }
        void report(int &result) const noexcept { result = value; }
        void take(std::unique_ptr<int> amount) { value += *amount; }
        void fail() { throw el::err::RuntimeError{"method"}; }
        void releaseOwner(std::shared_ptr<Recipient> &owner, bool &retained) {
            const auto weak = std::weak_ptr<Recipient>{owner};
            owner.reset();
            value = 42;
            retained = !weak.expired();
        }
    };
    struct DerivedRecipient final : Recipient {};

    void drain(const EventLoopPtr &loop) {
        (void)loop->runUntilIdle();
        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
    }

public:
    void testExplicitWeakMemberAndCopies() {
        const auto loop = EventLoop::create();
        const auto recipient = std::make_shared<DerivedRecipient>();
        const auto callback = bindWeakEvent(loop, std::weak_ptr<DerivedRecipient>{recipient}, &Recipient::add);
        const auto copy = callback;
        const auto erased = std::function<void(int)>{copy};
        callback(1);
        copy(2);
        erased(3);
        REQUIRE_EQUAL(recipient->value, 0);
        drain(loop);
        REQUIRE_EQUAL(recipient->value, 6);
        int reported = 0;
        bindWeakEvent(loop, std::weak_ptr<const Recipient>{recipient}, &Recipient::report)(std::ref(reported));
        bindWeakEvent(loop, std::weak_ptr<Recipient>{recipient}, &Recipient::take)(std::make_unique<int>(4));
        drain(loop);
        REQUIRE_EQUAL(reported, 6);
        REQUIRE_EQUAL(recipient->value, 10);
    }

    void testCurrentTargetIsCapturedAtBinding() {
        const auto receiver = EventLoop::create();
        const auto sender = EventLoop::create();
        const auto recipient = std::make_shared<Recipient>();
        auto callback = std::function<void(int)>{};
        receiver->invoke([&]() -> void {
            callback = bindWeakEvent(std::weak_ptr<Recipient>{recipient}, &Recipient::add);
            callback(1);
            REQUIRE_EQUAL(recipient->value, 0);
        });
        drain(receiver);
        sender->invoke([&]() -> void { callback(2); });
        drain(sender);
        REQUIRE_EQUAL(recipient->value, 1);
        drain(receiver);
        REQUIRE_EQUAL(recipient->value, 3);
    }

    void testEmptyAndExpiredRecipientsStillQueue() {
        const auto loop = EventLoop::create();
        auto recipient = std::make_shared<Recipient>();
        const auto callback = bindWeakEvent(loop, std::weak_ptr<Recipient>{recipient}, &Recipient::add);
        recipient.reset();
        callback(1);
        bindWeakEvent(loop, std::weak_ptr<Recipient>{}, &Recipient::add)(2);
        REQUIRE_EQUAL(loop->queuedEventCount(), std::size_t{2});
        drain(loop);
    }

    void testExpirationBetweenSubmissionAndDispatch() {
        const auto loop = EventLoop::create();
        auto recipient = std::make_shared<Recipient>();
        const auto weak = std::weak_ptr<Recipient>{recipient};
        int result = -1;
        {
            const auto callback = bindWeakEvent(loop, weak, &Recipient::report);
            callback(std::ref(result));
        }
        recipient.reset();
        REQUIRE(weak.expired());
        drain(loop);
        REQUIRE_EQUAL(result, -1);
    }

    void testLockRetainsRecipientThroughMethod() {
        const auto loop = EventLoop::create();
        auto recipient = std::make_shared<Recipient>();
        const auto weak = std::weak_ptr<Recipient>{recipient};
        bool retained = false;
        {
            const auto callback = bindWeakEvent(loop, weak, &Recipient::releaseOwner);
            callback(std::ref(recipient), std::ref(retained));
        }
        drain(loop);
        REQUIRE(retained);
        REQUIRE(recipient == nullptr);
        REQUIRE(weak.expired());
    }

    void testInvalidBindingsAndConstraints() {
        const auto loop = EventLoop::create();
        const auto weak = std::weak_ptr<Recipient>{};
        const auto nullMethod = static_cast<void (Recipient::*)(int)>(nullptr);
        REQUIRE_THROWS_AS(el::err::ParameterFailure, bindWeakEvent(EventTargetPtr{}, weak, &Recipient::add));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, bindWeakEvent(loop, weak, nullMethod));
        REQUIRE_THROWS_AS(el::err::LogicFailure, bindWeakEvent(weak, &Recipient::add));
        const auto wrongRecipient = bindWeakEvent(loop, std::weak_ptr<int>{}, &Recipient::add);
        const auto wrongConst = bindWeakEvent(loop, std::weak_ptr<const Recipient>{}, &Recipient::add);
        const auto callback = bindWeakEvent(loop, weak, &Recipient::add);
        static_assert(!std::is_invocable_v<decltype(wrongRecipient), int>);
        static_assert(!std::is_invocable_v<decltype(wrongConst), int>);
        static_assert(!std::is_invocable_v<decltype(callback), std::unique_ptr<int>>);
    }

    void testHandlerExceptionUsesLoopPolicy() {
        const auto loop = EventLoop::create();
        const auto recipient = std::make_shared<Recipient>();
        const auto callback = bindWeakEvent(loop, std::weak_ptr<Recipient>{recipient}, &Recipient::fail);
        REQUIRE_NOTHROW(callback());
        (void)loop->runUntilIdle();
        REQUIRE(loop->hasError());
        REQUIRE_THROWS_AS(el::err::RuntimeError, std::rethrow_exception(loop->takeError()));
        REQUIRE_FALSE(loop->hasError());
    }
};
