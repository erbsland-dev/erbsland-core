// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopDriver.hpp>
#include <erbsland/HostLookup.hpp>
#include <erbsland/HostLookupEventEditor.hpp>
#include <erbsland/HostLookupOptions.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/network/impl/host/HostResolver.hpp>
#include <erbsland/network/impl/host/HostResolverErrorContext.hpp>
#include <erbsland/network/impl/NetworkBackend.hpp>
#include <erbsland/NetworkError.hpp>
#include <erbsland/NetworkErrorReason.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/PlatformError.hpp>
#include <erbsland/PlatformErrorCategory.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/TimeDelta.hpp>
#include <erbsland/TimeUnitTags.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace el::event;
using namespace el::network;
using namespace el::text::literals;
using namespace el::time;

TESTED_TARGETS(
    Network HostLookup HostLookupEventEditor HostLookupOptions NetworkError NetworkErrorContext NetworkErrorReason
        NetworkBackend HostResolver)
class HostLookupTest final : public el::UnitTest {
    class FakeResolver final : public el::network::impl::HostResolver {
    public:
        using ResolveFn = std::function<el::util::List<IpAddress>(const HostName &)>;

        explicit FakeResolver(ResolveFn resolve) : _resolve{std::move(resolve)} {}

        auto resolve(const HostName &hostName) -> el::util::List<IpAddress> override {
            calls.fetch_add(1);
            return _resolve(hostName);
        }

        std::atomic<std::size_t> calls{0U};

    private:
        ResolveFn _resolve;
    };

public:
    void testReservedCompletionAndCancellationUnderQueueSaturation() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto loop = EventLoop::create(
            EventLoopDriver::createDefault(),
            EventLoopOptions{}.setMaximumQueuedEvents(1U).setQueueOverflowAction(EventQueueOverflowAction::Ignore));
        loop->registerBackend(
            std::make_unique<el::network::impl::NetworkBackend>(resolverReturning({IpAddress::loopbackV4()})));
        auto lookup = HostLookupPtr{};
        auto resolved = false;
        auto finalized = false;
        auto ordinary = false;
        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onResolved([&](const el::util::List<IpAddress> &) -> void { resolved = true; })
                .onFinal([&]() -> void { finalized = true; });
            loop->invoke([&]() -> void { ordinary = true; });
            lookup->start(IpAddress::loopbackV4());
            lookup->cancel();
        });
        runUntil(loop, [&]() -> bool { return finalized; });
        REQUIRE_FALSE(resolved);
        static_cast<void>(loop->runUntilIdle());
        REQUIRE(ordinary);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCompletedLookupsReleaseDeadlineAdmissionPromptly() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto loop = loopWithResolver(resolverReturning({IpAddress::loopbackV4()}));
        auto lookup = HostLookupPtr{};
        auto completed = std::size_t{};
        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events().onFinal([&]() -> void {
                if (++completed < 1100U) {
                    lookup->start(IpAddress::loopbackV4(), HostLookupOptions{}.setTimeout(Seconds{60}));
                }
            });
            lookup->start(IpAddress::loopbackV4(), HostLookupOptions{}.setTimeout(Seconds{60}));
        });
        runUntil(loop, [&]() -> bool { return completed == 1100U; });
        REQUIRE_FALSE(loop->hasError());
    }

    void testOptionDefaultsAndSetters() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = HostLookupOptions{};
        REQUIRE_EQUAL(options.timeout(), TimeDelta::seconds(10));
        REQUIRE_EQUAL(options.maximumAttempts(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(options.retryDelay(), TimeDelta::milliseconds(100));

        options.setTimeout(Seconds{3}).setMaximumAttempts(el::unit::ItemCount{4U}).setRetryDelay(Milliseconds{25});
        REQUIRE_EQUAL(options.timeout(), TimeDelta::seconds(3));
        REQUIRE_EQUAL(options.maximumAttempts(), el::unit::ItemCount{4U});
        REQUIRE_EQUAL(options.retryDelay(), TimeDelta::milliseconds(25));
    }

    void testNumericLookupIsAsynchronousAndFinalized() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto resolver = resolverReturning({IpAddress::loopbackV6()});
        const auto loop = loopWithResolver(resolver);
        auto lookup = HostLookupPtr{};
        auto events = std::vector<int>{};
        const auto expected = IpAddress::fromStringOrThrow("192.0.2.44"_el);

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            REQUIRE_FALSE(lookup->host().has_value());
            lookup->events()
                .onResolved([&](const el::util::List<IpAddress> &addresses) -> void {
                    REQUIRE_EQUAL(lookup->state(), NetworkSourceState::Closed);
                    REQUIRE_EQUAL(lookup->host(), std::optional<Host>{expected});
                    REQUIRE_EQUAL(addresses, el::util::List<IpAddress>{expected});
                    events.push_back(1);
                })
                .onFinal([&]() -> void {
                    REQUIRE_EQUAL(lookup->state(), NetworkSourceState::Inactive);
                    REQUIRE_FALSE(lookup->host().has_value());
                    events.push_back(2);
                });
            lookup->start(expected);
            REQUIRE_EQUAL(lookup->state(), NetworkSourceState::Starting);
            REQUIRE(lookup->host().has_value());
            REQUIRE(events.empty());
        });

        REQUIRE(loop->runOnce());
        REQUIRE(events.empty());
        REQUIRE(loop->runOnce());
        REQUIRE_EQUAL(events, std::vector<int>({1, 2}));
        REQUIRE_EQUAL(lookup->state(), NetworkSourceState::Inactive);
        REQUIRE_EQUAL(resolver->calls.load(), std::size_t{0U});
    }

    void testLookupCanBeReusedFromFinal() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto resolvedAddress = IpAddress::fromStringOrThrow("192.0.2.10"_el);
        const auto resolver = resolverReturning({resolvedAddress});
        const auto loop = loopWithResolver(resolver);
        auto lookup = HostLookupPtr{};
        auto resolvedCount = 0;
        auto finalCount = 0;

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onResolved([&](const el::util::List<IpAddress> &) -> void { resolvedCount += 1; })
                .onFinal([&]() -> void {
                    finalCount += 1;
                    if (finalCount == 1) {
                        lookup->start(Host::fromStringOrThrow("second.example"_el));
                    }
                });
            lookup->start(IpAddress::loopbackV4());
        });

        REQUIRE(loop->runOnce());
        runUntil(loop, [&]() -> bool { return finalCount == 2; });
        REQUIRE_EQUAL(resolvedCount, 2);
        REQUIRE_EQUAL(finalCount, 2);
        REQUIRE_EQUAL(resolver->calls.load(), std::size_t{1U});
        REQUIRE_EQUAL(lookup->state(), NetworkSourceState::Inactive);
    }

    void testNamedLookupPreservesOrderAndRemovesDuplicates() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto first = IpAddress::fromStringOrThrow("2001:db8::10"_el);
        const auto second = IpAddress::fromStringOrThrow("192.0.2.10"_el);
        const auto resolver = resolverReturning({first, second, first, second});
        const auto loop = loopWithResolver(resolver);
        auto lookup = HostLookupPtr{};
        auto result = el::util::List<IpAddress>{};
        auto finalized = false;

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onResolved([&](const auto &addresses) -> void { result = addresses; })
                .onFinal([&]() -> void { finalized = true; });
            lookup->start(Host::fromStringOrThrow("moon.example"_el));
        });

        REQUIRE(loop->runOnce());
        runUntil(loop, [&]() -> bool { return finalized; });
        REQUIRE_EQUAL(result, (el::util::List<IpAddress>{first, second}));
        REQUIRE_EQUAL(resolver->calls.load(), std::size_t{1U});
    }

    void testNotFoundProducesStructuredContextWithoutRetry() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto resolver = resolverThrowing(el::system::PlatformErrorCategory::NotFound, false, "host not found"_el);
        const auto loop = loopWithResolver(resolver);
        auto lookup = HostLookupPtr{};
        auto context = std::optional<NetworkErrorContext>{};
        auto finalized = false;

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onError([&](const NetworkErrorContext &error) -> void {
                    REQUIRE_EQUAL(lookup->state(), NetworkSourceState::Failed);
                    context = error;
                })
                .onFinal([&]() -> void { finalized = true; });
            lookup->start(Host::fromStringOrThrow("missing.example"_el));
        });

        REQUIRE(loop->runOnce());
        runUntil(loop, [&]() -> bool { return finalized; });
        REQUIRE(context.has_value());
        REQUIRE_EQUAL(context->reason(), NetworkErrorReason::HostNotFound);
        REQUIRE(context->host().has_value());
        REQUIRE_EQUAL(context->host()->toString(), "missing.example"_el);
        REQUIRE(context->platformContext());
        REQUIRE_EQUAL(context->platformContext()->category(), el::system::PlatformErrorCategory::NotFound);
        REQUIRE_EQUAL(NetworkError{*context}.diagnostic()->toString(), "Host lookup failed"_el);
        REQUIRE_EQUAL(resolver->calls.load(), std::size_t{1U});
    }

    void testEmptyResultHasNoAddressesReason() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto resolver = resolverReturning({});
        const auto loop = loopWithResolver(resolver);
        auto lookup = HostLookupPtr{};
        auto reason = NetworkErrorReason::Unknown;
        auto finalized = false;

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onError([&](const NetworkErrorContext &error) -> void { reason = error.reason(); })
                .onFinal([&]() -> void { finalized = true; });
            lookup->start(Host::fromStringOrThrow("empty.example"_el));
        });

        REQUIRE(loop->runOnce());
        runUntil(loop, [&]() -> bool { return finalized; });
        REQUIRE_EQUAL(reason, NetworkErrorReason::NoAddresses);
    }

    void testTransientFailureRetriesAfterDelay() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto address = IpAddress::fromStringOrThrow("192.0.2.21"_el);
        auto resolver =
            std::make_shared<FakeResolver>([address, call = 0](const HostName &) mutable -> el::util::List<IpAddress> {
                call += 1;
                if (call == 1) {
                    throwTemporaryResolverError();
                }
                return el::util::List<IpAddress>{address};
            });
        const auto loop = loopWithResolver(resolver);
        auto lookup = HostLookupPtr{};
        auto resolved = false;
        auto finalized = false;

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onResolved([&](const el::util::List<IpAddress> &) -> void { resolved = true; })
                .onFinal([&]() -> void { finalized = true; });
            lookup->start(
                Host::fromStringOrThrow("retry.example"_el),
                HostLookupOptions{}.setTimeout(Seconds{1}).setRetryDelay(Milliseconds{5}));
        });

        REQUIRE(loop->runOnce());
        runUntil(loop, [&]() -> bool { return finalized; });
        REQUIRE(resolved);
        REQUIRE_EQUAL(resolver->calls.load(), std::size_t{2U});
    }

    void testTransientFailureStopsAtMaximumAttempts() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto resolver = std::make_shared<FakeResolver>(
            [](const HostName &) -> el::util::List<IpAddress> { throwTemporaryResolverError(); });
        const auto loop = loopWithResolver(resolver);
        auto lookup = HostLookupPtr{};
        auto reason = NetworkErrorReason::Unknown;
        auto finalized = false;

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onError([&](const NetworkErrorContext &error) -> void { reason = error.reason(); })
                .onFinal([&]() -> void { finalized = true; });
            lookup->start(
                Host::fromStringOrThrow("retry.example"_el),
                HostLookupOptions{}
                    .setTimeout(Seconds{1})
                    .setMaximumAttempts(el::unit::ItemCount{2U})
                    .setRetryDelay(Milliseconds{5}));
        });

        REQUIRE(loop->runOnce());
        runUntil(loop, [&]() -> bool { return finalized; });
        REQUIRE_EQUAL(reason, NetworkErrorReason::HostResolutionFailed);
        REQUIRE_EQUAL(resolver->calls.load(), std::size_t{2U});
    }

    void testTimeoutWinsDuringRetryDelay() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto resolver = std::make_shared<FakeResolver>(
            [](const HostName &) -> el::util::List<IpAddress> { throwTemporaryResolverError(); });
        const auto loop = loopWithResolver(resolver);
        auto lookup = HostLookupPtr{};
        auto reason = NetworkErrorReason::Unknown;
        auto finalized = false;

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onError([&](const NetworkErrorContext &error) -> void { reason = error.reason(); })
                .onFinal([&]() -> void { finalized = true; });
            lookup->start(
                Host::fromStringOrThrow("slow-retry.example"_el),
                HostLookupOptions{}
                    .setTimeout(Milliseconds{30})
                    .setMaximumAttempts(el::unit::ItemCount{2U})
                    .setRetryDelay(Seconds{1}));
        });

        REQUIRE(loop->runOnce());
        runUntil(loop, [&]() -> bool { return finalized; });
        REQUIRE_EQUAL(reason, NetworkErrorReason::Timeout);
        REQUIRE_EQUAL(resolver->calls.load(), std::size_t{1U});
    }

    void testTimeoutSuppressesLateResolverResult() {
        auto applicationScope = ApplicationTestScope<>{};
        auto entered = false;
        auto release = false;
        auto finished = std::atomic<bool>{false};
        auto mutex = std::mutex{};
        auto condition = std::condition_variable{};
        const auto resolver = std::make_shared<FakeResolver>([&](const HostName &) -> el::util::List<IpAddress> {
            auto lock = std::unique_lock{mutex};
            entered = true;
            condition.notify_all();
            condition.wait(lock, [&]() -> bool { return release; });
            finished = true;
            return el::util::List<IpAddress>{IpAddress::loopbackV4()};
        });
        const auto loop = loopWithResolver(resolver);
        auto lookup = HostLookupPtr{};
        auto resolvedCount = 0;
        auto reason = NetworkErrorReason::Unknown;
        auto finalCount = 0;

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onResolved([&](const el::util::List<IpAddress> &) -> void { resolvedCount += 1; })
                .onError([&](const NetworkErrorContext &error) -> void { reason = error.reason(); })
                .onFinal([&]() -> void { finalCount += 1; });
            lookup->start(
                Host::fromStringOrThrow("blocked.example"_el), HostLookupOptions{}.setTimeout(Milliseconds{30}));
        });
        REQUIRE(loop->runOnce());
        {
            auto lock = std::unique_lock{mutex};
            condition.wait(lock, [&]() -> bool { return entered; });
        }
        runUntil(loop, [&]() -> bool { return finalCount == 1; });
        REQUIRE_EQUAL(reason, NetworkErrorReason::Timeout);

        {
            const auto lock = std::scoped_lock{mutex};
            release = true;
        }
        condition.notify_all();
        while (!finished.load()) {
            std::this_thread::yield();
        }
        static_cast<void>(loop->runUntilIdle());
        REQUIRE_EQUAL(resolvedCount, 0);
        REQUIRE_EQUAL(finalCount, 1);
    }

    void testCancellationEmitsOnlyFinalAndAllowsReuse() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto resolver = resolverReturning({IpAddress::loopbackV4()});
        const auto loop = loopWithResolver(resolver);
        auto lookup = HostLookupPtr{};
        auto resolvedCount = 0;
        auto errorCount = 0;
        auto finalCount = 0;

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onResolved([&](const el::util::List<IpAddress> &) -> void { resolvedCount += 1; })
                .onError([&](const NetworkErrorContext &) -> void { errorCount += 1; })
                .onFinal([&]() -> void {
                    finalCount += 1;
                    if (finalCount == 1) {
                        lookup->start(IpAddress::loopbackV6());
                    }
                });
            lookup->cancel();
            lookup->start(IpAddress::loopbackV4());
            lookup->cancel();
            lookup->cancel();
        });

        REQUIRE(loop->runOnce());
        runUntil(loop, [&]() -> bool { return finalCount == 2; });
        REQUIRE_EQUAL(resolvedCount, 1);
        REQUIRE_EQUAL(errorCount, 0);
        REQUIRE_EQUAL(finalCount, 2);
    }

    void testCancellationIsThreadSafe() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loop = loopWithResolver(resolverReturning({IpAddress::loopbackV4()}));
        auto lookup = HostLookupPtr{};
        auto resolvedCount = 0;
        auto errorCount = 0;
        auto finalCount = 0;

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onResolved([&](const el::util::List<IpAddress> &) -> void { resolvedCount += 1; })
                .onError([&](const NetworkErrorContext &) -> void { errorCount += 1; })
                .onFinal([&]() -> void { finalCount += 1; });
            lookup->start(IpAddress::loopbackV4());
        });
        REQUIRE(loop->runOnce());

        auto cancelThread = std::thread{[lookup]() -> void { lookup->cancel(); }};
        cancelThread.join();
        runUntil(loop, [&]() -> bool { return finalCount == 1; });
        REQUIRE_EQUAL(resolvedCount, 0);
        REQUIRE_EQUAL(errorCount, 0);
        REQUIRE_EQUAL(finalCount, 1);
        REQUIRE_EQUAL(lookup->state(), NetworkSourceState::Inactive);
        REQUIRE_FALSE(lookup->host().has_value());
    }

    void testHandlerExceptionSkipsFinalAndRestoresInactiveState() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto resolver = resolverReturning({});
        const auto loop = loopWithResolver(resolver);
        auto lookup = HostLookupPtr{};
        auto finalCalled = false;

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onResolved([](const el::util::List<IpAddress> &) -> void { throw el::err::RuntimeError{"callback"}; })
                .onFinal([&]() -> void { finalCalled = true; });
            lookup->start(IpAddress::loopbackV4());
        });
        REQUIRE(loop->runOnce());
        REQUIRE(loop->runOnce());
        REQUIRE(loop->hasError());
        REQUIRE_FALSE(finalCalled);
        REQUIRE_EQUAL(lookup->state(), NetworkSourceState::Inactive);
        REQUIRE_FALSE(lookup->host().has_value());
    }

    void testErrorContextCanBeThrownAsNetworkError() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto resolver = resolverReturning({});
        const auto loop = loopWithResolver(resolver);
        auto lookup = HostLookupPtr{};
        auto finalCalled = false;

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onError([](const NetworkErrorContext &context) -> void { throw NetworkError{context}; })
                .onFinal([&]() -> void { finalCalled = true; });
            lookup->start(Host::fromStringOrThrow("empty.example"_el));
        });
        REQUIRE(loop->runOnce());
        runUntil(loop, [&]() -> bool { return loop->hasError(); });
        REQUIRE_FALSE(finalCalled);
        auto caught = false;
        try {
            std::rethrow_exception(loop->takeError());
        } catch (const NetworkError &error) {
            caught = error.context().reason() == NetworkErrorReason::NoAddresses;
        }
        REQUIRE(caught);
    }

    void testFinalExceptionPropagatesAfterFinalization() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loop = loopWithResolver(resolverReturning({IpAddress::loopbackV4()}));
        auto lookup = HostLookupPtr{};

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            lookup->events().onFinal([]() -> void { throw el::err::RuntimeError{"final callback"}; });
            lookup->start(IpAddress::loopbackV4());
        });
        REQUIRE(loop->runOnce());
        REQUIRE(loop->runOnce());
        REQUIRE(loop->hasError());
        REQUIRE_EQUAL(lookup->state(), NetworkSourceState::Inactive);
        REQUIRE_FALSE(lookup->host().has_value());
        REQUIRE_THROWS_AS(el::err::RuntimeError, std::rethrow_exception(loop->takeError()));
    }

    void testStartAndEditorRequireOwnerLoop() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto resolver = resolverReturning({IpAddress::loopbackV4()});
        const auto ownerLoop = loopWithResolver(resolver);
        const auto otherLoop = EventLoop::create();
        auto lookup = HostLookupPtr{};
        ownerLoop->invoke([&]() -> void { lookup = ownerLoop->get<Network>().createHostLookup(); });
        REQUIRE(ownerLoop->runOnce());
        REQUIRE_THROWS_AS(el::err::LogicFailure, lookup->start(IpAddress::loopbackV4()));
        REQUIRE_THROWS_AS(el::err::LogicFailure, lookup->events());
        otherLoop->invoke([&]() -> void {
            REQUIRE_THROWS_AS(el::err::LogicFailure, lookup->start(IpAddress::loopbackV4()));
            REQUIRE_THROWS_AS(el::err::LogicFailure, lookup->events());
        });
        REQUIRE(otherLoop->runOnce());
    }

    void testCreationRequiresCurrentOwnerLoop() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loop = loopWithResolver(resolverReturning({IpAddress::loopbackV4()}));
        REQUIRE_THROWS_AS(el::err::LogicFailure, loop->get<Network>().createHostLookup());
    }

    void testInvalidOptionsAndConcurrentStartAreRejected() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loop = loopWithResolver(resolverReturning({IpAddress::loopbackV4()}));
        auto checked = false;
        loop->invoke([&]() -> void {
            const auto lookup = loop->get<Network>().createHostLookup();
            REQUIRE_THROWS_AS(
                el::err::ParameterFailure,
                lookup->start(IpAddress::loopbackV4(), HostLookupOptions{}.setTimeout(TimeDelta::zero())));
            REQUIRE_THROWS_AS(
                el::err::ParameterFailure,
                lookup->start(IpAddress::loopbackV4(), HostLookupOptions{}.setMaximumAttempts(el::unit::ItemCount{})));
            REQUIRE_THROWS_AS(
                el::err::ParameterFailure,
                lookup->start(IpAddress::loopbackV4(), HostLookupOptions{}.setRetryDelay(TimeDelta::milliseconds(-1))));
            lookup->start(IpAddress::loopbackV4());
            REQUIRE_THROWS_AS(el::err::LogicFailure, lookup->start(IpAddress::loopbackV6()));
            checked = true;
        });
        REQUIRE(loop->runOnce());
        REQUIRE(checked);
    }

    void testEditorIsStableAndHandlersAreReplaceable() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loop = loopWithResolver(resolverReturning({IpAddress::loopbackV4()}));
        auto lookup = HostLookupPtr{};
        auto calls = 0;
        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup();
            auto &first = lookup->events();
            auto &second = lookup->events();
            REQUIRE_EQUAL(&first, &second);
            REQUIRE_EQUAL(first.source(), lookup);
            REQUIRE_EQUAL(first.target(), loop);
            first.onResolved([&](const el::util::List<IpAddress> &) -> void { calls += 1; })
                .onResolved([&](const el::util::List<IpAddress> &) -> void { calls += 10; })
                .onFinal({});
            lookup->start(IpAddress::loopbackV4());
        });
        REQUIRE(loop->runOnce());
        REQUIRE(loop->runOnce());
        REQUIRE_EQUAL(calls, 10);
    }

    void testDestructionSuppressesQueuedCompletion() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loop = loopWithResolver(resolverReturning({IpAddress::loopbackV4()}));
        auto callbackCalled = false;
        loop->invoke([&]() -> void {
            auto lookup = loop->get<Network>().createHostLookup();
            lookup->events()
                .onResolved([&](const el::util::List<IpAddress> &) -> void { callbackCalled = true; })
                .onFinal([&]() -> void { callbackCalled = true; });
            lookup->start(IpAddress::loopbackV4());
            lookup.reset();
        });
        REQUIRE(loop->runOnce());
        REQUIRE(loop->runOnce());
        REQUIRE_FALSE(callbackCalled);
    }

private:
    template <typename Predicate>
    void runUntil(const EventLoopPtr &loop, Predicate predicate) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
        while (!predicate() && std::chrono::steady_clock::now() < deadline) {
            static_cast<void>(loop->runOnce(TimeDelta{Milliseconds{50}}));
        }
        REQUIRE(predicate());
    }

    [[noreturn]] static void throwTemporaryResolverError() {
        throw el::system::PlatformError{
            "temporary resolver failure"_el,
            std::make_shared<el::network::impl::HostResolverErrorContext>(
                7, "try again"_el, el::system::PlatformErrorCategory::Unknown, true)};
    }

    [[nodiscard]] static auto resolverReturning(std::initializer_list<IpAddress> addresses)
        -> std::shared_ptr<FakeResolver> {
        const auto result = el::util::List<IpAddress>{addresses};
        return std::make_shared<FakeResolver>(
            [result](const HostName &) -> el::util::List<IpAddress> { return result; });
    }

    [[nodiscard]] static auto resolverThrowing(
        const el::system::PlatformErrorCategory category, const bool isRetryable, el::text::String message)
        -> std::shared_ptr<FakeResolver> {
        return std::make_shared<FakeResolver>(
            [category, isRetryable, message = std::move(message)](const HostName &) -> el::util::List<IpAddress> {
                throw el::system::PlatformError{
                    "simulated resolver failure"_el,
                    std::make_shared<el::network::impl::HostResolverErrorContext>(7, message, category, isRetryable)};
            });
    }

    [[nodiscard]] static auto loopWithResolver(const std::shared_ptr<FakeResolver> &resolver) -> EventLoopPtr {
        auto loop = EventLoop::create();
        loop->registerBackend(std::make_unique<el::network::impl::NetworkBackend>(resolver));
        return loop;
    }
};
