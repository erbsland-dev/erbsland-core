#include "../core/ApplicationTestScope.hpp"

#include <erbsland/ByteBuffer.hpp>
#include <erbsland/core/impl/application_data/ApplicationCryptologyData.hpp>
#include <erbsland/core/impl/application_data/ApplicationRandomData.hpp>
#include <erbsland/core/impl/ApplicationDataAccess.hpp>
#include <erbsland/core/impl/ApplicationExecutionContext.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/event/impl/EventContinuation.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopDriver.hpp>
#include <erbsland/network/impl/tls/TlsBackgroundOperation.hpp>
#include <erbsland/ProtectedByteBlock.hpp>
#include <erbsland/ProtectedDataMode.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/impl/worker_executor/WorkerAdmissionError.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <thread>

TESTED_TARGETS(TlsBackgroundOperation EventContinuation ApplicationExecutionContext)
class TlsBackgroundOperationTest final : public el::UnitTest {
    struct Request final {
        std::function<int()> work;
        [[nodiscard]] auto execute() const -> int { return work(); }
    };

    template <typename Predicate>
    void runUntil(const el::event::EventLoopPtr &loop, Predicate predicate) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
        while (!predicate() && std::chrono::steady_clock::now() < deadline) {
            static_cast<void>(loop->runOnce(el::time::Milliseconds{1}));
        }
        REQUIRE(predicate());
    }

public:
    void testCapturedServicesSurviveCancelledApplicationScope() {
        const auto entered = std::make_shared<std::promise<void>>();
        auto entry = entered->get_future();
        const auto release = std::make_shared<std::promise<void>>();
        const auto gate = release->get_future().share();
        const auto observed = std::make_shared<std::promise<bool>>();
        auto observation = observed->get_future();
        const auto restored = std::make_shared<std::promise<bool>>();
        auto restoration = restored->get_future();
        auto lifetime = std::shared_ptr<int>{new int{}, [restored](int *value) {
                                                 delete value;
                                                 restored->set_value(
                                                     el::core::impl::ApplicationExecutionContext::current() == nullptr);
                                             }};
        auto operation = el::network::impl::TlsBackgroundOperation{};
        auto completed = std::atomic<bool>{false};
        auto enteredStatus = std::future_status::timeout;
        auto loop = el::event::EventLoopPtr{};
        {
            auto applicationScope = ApplicationTestScope<>{};
            auto &configuration = applicationScope.app().cryptologyConfiguration();
            configuration.setProtectedDataMode(el::cryptology::ProtectedDataMode::InternalOnly);
            const auto originalConfiguration = &configuration;
            const auto originalRandom = &applicationScope.app().secureRandom();
            const auto plaintext = el::mem::ByteBuffer{0x01, 0x23, 0x45, 0x67};
            const auto protectedValue = el::cryptology::ProtectedByteBlock{plaintext.span()};
            loop = el::event::EventLoop::create(el::event::EventLoopDriver::createDefault());
            operation.start(
                Request{
                    [entered,
                        gate,
                        observed,
                        lifetime,
                        originalConfiguration,
                        originalRandom,
                        protectedValue,
                        plaintext]() -> int {
                        entered->set_value();
                        gate.wait();
                        auto valid = false;
                        try {
                            const auto data = el::core::impl::applicationData();
                            valid = &data->cryptology().get()->configuration() == originalConfiguration &&
                                &data->random().get()->secureRandom() == originalRandom &&
                                data->cryptology().get()->configuration().protectedDataMode() ==
                                    el::cryptology::ProtectedDataMode::InternalOnly &&
                                protectedValue.unprotect().isEqualConstTime(plaintext.span());
                            const auto replacement = el::cryptology::ProtectedByteBlock{plaintext.span()};
                            valid = valid && replacement.unprotect().isEqualConstTime(plaintext.span());
                        } catch (...) {
                            valid = false;
                        }
                        observed->set_value(valid);
                        return *lifetime;
                    }},
                loop,
                el::time::TimePoint::inFuture(el::time::Seconds{10}),
                [&](std::optional<int>, el::text::String) -> void { completed.store(true); });
            lifetime.reset();
            enteredStatus = entry.wait_for(std::chrono::seconds{2});
            operation.cancel();
        }
        auto replacementScope = ApplicationTestScope<>{};
        replacementScope.app().cryptologyConfiguration().setProtectedDataMode(
            el::cryptology::ProtectedDataMode::PlatformOnly);
        release->set_value();
        REQUIRE_EQUAL(enteredStatus, std::future_status::ready);
        REQUIRE_EQUAL(observation.wait_for(std::chrono::seconds{2}), std::future_status::ready);
        REQUIRE(observation.get());
        REQUIRE_EQUAL(restoration.wait_for(std::chrono::seconds{2}), std::future_status::ready);
        REQUIRE(restoration.get());
        REQUIRE(el::core::impl::ApplicationExecutionContext::current() == nullptr);
        REQUIRE(&el::core::application() == &replacementScope.app());
        REQUIRE_FALSE(completed.load());
    }

    void testNestedApplicationContextsRestoreTheirPredecessor() {
        auto firstData = el::core::impl::ApplicationDataPtr{};
        {
            auto applicationScope = ApplicationTestScope<>{};
            applicationScope.app().cryptologyConfiguration().setProtectedDataMode(
                el::cryptology::ProtectedDataMode::InternalOnly);
            firstData = el::core::impl::applicationData();
        }
        auto secondScope = ApplicationTestScope<>{};
        const auto secondData = el::core::impl::applicationData();
        {
            const auto outer = el::core::impl::ApplicationExecutionContext{firstData};
            REQUIRE(&el::core::application() == &secondScope.app());
            REQUIRE(
                el::core::impl::applicationData()->cryptology().get()->configuration().protectedDataMode() ==
                el::cryptology::ProtectedDataMode::InternalOnly);
            try {
                const auto inner = el::core::impl::ApplicationExecutionContext{secondData};
                REQUIRE(&el::core::Application::instance() == &secondScope.app());
                REQUIRE(el::core::impl::applicationData() == secondData);
                throw 42;
            } catch (const int) {}
            REQUIRE(&el::core::application() == &secondScope.app());
            REQUIRE(el::core::impl::applicationData() == firstData);
        }
        REQUIRE(el::core::impl::ApplicationExecutionContext::current() == nullptr);
        REQUIRE(&el::core::application() == &secondScope.app());
    }

    void testCancelledTimersReleaseAdmissionBeforeDeadline() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto loop = el::event::EventLoop::create(el::event::EventLoopDriver::createDefault());
        auto called = false;
        for (auto index = std::size_t{}; index < 1100U; ++index) {
            auto continuation = el::event::impl::EventContinuation::create(loop);
            continuation->invokeAfter(el::time::Seconds{60}, [&]() -> void { called = true; });
            continuation->cancel();
            continuation.reset();
        }
        static_cast<void>(loop->runUntilIdle());
        REQUIRE_FALSE(called);
        const auto continuation = el::event::impl::EventContinuation::create(loop);
        continuation->invokeAfter(el::time::Seconds{60}, []() -> void {});
        REQUIRE_THROWS_AS(el::err::LogicFailure, continuation->invokeAfter(el::time::Seconds{60}, []() -> void {}));
        continuation->cancel();
    }

    void testReservedCompletionAndDeadlineSurviveFullOrdinaryQueue() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto loop = el::event::EventLoop::create(
            el::event::EventLoopDriver::createDefault(),
            el::event::EventLoopOptions{}.setMaximumQueuedEvents(1U).setQueueOverflowAction(
                el::event::EventQueueOverflowAction::Ignore));
        const auto completion = el::event::impl::EventContinuation::create(loop);
        const auto deadline = el::event::impl::EventContinuation::create(loop);
        auto ordinary = false;
        auto completed = false;
        auto timedOut = false;
        loop->invoke([&]() -> void { ordinary = true; });
        completion->invoke([&]() -> void { completed = true; });
        deadline->invokeAfter(el::time::Milliseconds{1}, [&]() -> void { timedOut = true; });
        REQUIRE_EQUAL(loop->queuedEventCount(), std::size_t{2U});
        runUntil(loop, [&]() -> bool { return timedOut; });
        REQUIRE(completed);
        REQUIRE(ordinary);
    }

    void testExecutionLeavesOwnerThreadAndCompletionReturns() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto loop = el::event::EventLoop::create(el::event::EventLoopDriver::createDefault());
        const auto owner = std::this_thread::get_id();
        auto operation = el::network::impl::TlsBackgroundOperation{};
        auto completed = false;
        auto resultValue = 0;
        auto completionOnOwner = false;
        operation.start(
            Request{[owner]() -> int { return std::this_thread::get_id() != owner ? 42 : -1; }},
            loop,
            el::time::TimePoint::inFuture(el::time::Seconds{2}),
            [&](std::optional<int> result, el::text::String diagnostic) -> void {
                completionOnOwner = std::this_thread::get_id() == owner;
                resultValue = result.value_or(-2);
                completed = diagnostic.isEmpty();
                operation.cancel();
            });
        runUntil(loop, [&]() -> bool { return completed; });
        REQUIRE_EQUAL(resultValue, 42);
        REQUIRE(completionOnOwner);
        REQUIRE_FALSE(operation.isPending());
    }

    void testExpiredQueuedWorkDoesNotExecute() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto loop = el::event::EventLoop::create(el::event::EventLoopDriver::createDefault());
        auto called = std::atomic<bool>{false};
        auto operation = el::network::impl::TlsBackgroundOperation{};
        auto completed = false;
        auto hasResult = true;
        operation.start(
            Request{[&]() -> int {
                called.store(true);
                return 42;
            }},
            loop,
            el::time::TimePoint{},
            [&](std::optional<int> result, el::text::String) -> void {
                hasResult = result.has_value();
                completed = true;
                operation.cancel();
            });
        runUntil(loop, [&]() -> bool { return completed; });
        REQUIRE_FALSE(called.load());
        REQUIRE_FALSE(hasResult);
    }

    void testCancellationDiscardsLateResultWithoutBlockingOwner() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto loop = el::event::EventLoop::create(el::event::EventLoopDriver::createDefault());
        const auto entered = std::make_shared<std::promise<void>>();
        auto entry = entered->get_future();
        const auto release = std::make_shared<std::promise<void>>();
        const auto gate = release->get_future().share();
        auto lifetime = std::make_shared<int>(1);
        const auto weakLifetime = std::weak_ptr<int>{lifetime};
        auto completed = false;
        auto ownerResponsive = false;
        auto operation = el::network::impl::TlsBackgroundOperation{};
        operation.start(
            Request{[entered, gate, lifetime]() -> int {
                entered->set_value();
                gate.wait();
                return *lifetime;
            }},
            loop,
            el::time::TimePoint::inFuture(el::time::Seconds{2}),
            [&](std::optional<int>, el::text::String) -> void { completed = true; });
        lifetime.reset();
        const auto enteredStatus = entry.wait_for(std::chrono::seconds{2});
        operation.cancel();
        loop->invoke([&]() -> void { ownerResponsive = true; });
        static_cast<void>(loop->runUntilIdle());
        release->set_value();
        REQUIRE_EQUAL(enteredStatus, std::future_status::ready);
        REQUIRE(ownerResponsive);
        runUntil(loop, [&]() -> bool { return weakLifetime.expired(); });
        REQUIRE_FALSE(completed);
    }

    void testOverloadNeverRunsInline() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto loop = el::event::EventLoop::create(el::event::EventLoopDriver::createDefault());
        auto reservations = std::vector<el::util::impl::WorkerReservation>{};
        auto &executor = el::util::impl::worker_services::blocking();
        try {
            for (auto index = std::size_t{}; index < 1024U; ++index) {
                reservations.push_back(executor.reserve(el::util::impl::WorkerExecutor::WorkClass::Security));
            }
        } catch (const el::util::impl::WorkerAdmissionError &) {}
        auto called = false;
        auto operation = el::network::impl::TlsBackgroundOperation{};
        REQUIRE_THROWS_AS(
            el::util::impl::WorkerAdmissionError,
            operation.start(
                Request{[&]() -> int {
                    called = true;
                    return 42;
                }},
                loop,
                el::time::TimePoint::inFuture(el::time::Seconds{2}),
                [](std::optional<int>, el::text::String) -> void {}));
        REQUIRE_FALSE(called);
        REQUIRE_FALSE(operation.isPending());
        REQUIRE(executor.reserve().isValid());
    }
};
