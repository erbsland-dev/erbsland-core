// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../event/NativeEventLoopTestTypes.hpp"

#include <erbsland/network/impl/tcp/posix/PosixTcpConnectionDevice.hpp>
#include <erbsland/network/impl/tcp/posix/PosixTcpListenerDevice.hpp>
#include <erbsland/network/impl/tcp/posix/PosixTcpSocket.hpp>
#include <erbsland/network/impl/udp/posix/PosixUdpSocketDevice.hpp>
#include <erbsland/Subprocess.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <utility>

using namespace el::network;
using namespace el::network::impl;
using namespace el::unit;
using namespace el::text::literals;

TESTED_TARGETS(PosixSocketDevice PosixTcpConnectionDevice PosixTcpListenerDevice PosixUdpSocketDevice)
class PosixSocketLifecycleTest final : public el::UnitTest {
    using Registration = el::event::impl::EventLoopDriverRegistration;
    using Callback = std::function<void(bool, bool, bool)>;

    class RegistrationObserver {
    public:
        auto registerDescriptor(int descriptor, bool read, bool write, Callback callback)
            -> std::unique_ptr<Registration> {
            lastDescriptor = descriptor;
            savedCallback = callback;
            auto registration =
                std::shared_ptr<Registration>{driver->registerDescriptor(descriptor, read, write, std::move(callback))};
            if (duringRegistration) {
                duringRegistration();
            }
            return std::make_unique<Registration>(
                [this, descriptor, registration = std::move(registration)]() mutable noexcept -> void {
                    removedWhileOpen = ::fcntl(descriptor, F_GETFD) >= 0;
                    ++removals;
                    registration.reset();
                });
        }
        auto function() {
            return [this](int descriptor, bool read, bool write, Callback callback) {
                return registerDescriptor(descriptor, read, write, std::move(callback));
            };
        }
        std::shared_ptr<NativeTestDriver> driver{std::make_shared<NativeTestDriver>()};
        int lastDescriptor{-1};
        int removals{};
        bool removedWhileOpen{};
        Callback savedCallback;
        std::function<void()> duringRegistration;
    };

    static auto endpoint() -> IpEndpoint { return IpEndpoint{IpAddress::loopbackV4(), Port{}}; }

public:
    void testFactoriesEstablishSharedOwnership() {
        auto observer = RegistrationObserver{};
        const auto connection =
            PosixTcpConnectionDevice::create(observer.function(), ByteCount{256U}, TcpConnectionDeviceCallbacks{});
        const auto listener = PosixTcpListenerDevice::create(observer.function(), TcpListenerDeviceCallbacks{});
        const auto datagram = PosixUdpSocketDevice::create(observer.function(), UdpSocketDeviceCallbacks{});
        REQUIRE_EQUAL(connection->weak_from_this().lock(), connection);
        REQUIRE_EQUAL(listener->weak_from_this().lock(), listener);
        REQUIRE_EQUAL(datagram->weak_from_this().lock(), datagram);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testPosixLifecycleChildren() {
        for (
            const auto name :
            {"UdpAbortUnregistersBeforeCloseAndProtectsDescriptorReuse"_el,
                "AbortWaitsForRegistrationCommit"_el,
                "TcpSynchronousCallbackCanReleaseSourceOwnership"_el,
                "TcpCallbackCanAbortAndReleaseItsOwnerFromAnotherThread"_el,
                "ListenerCrossThreadAbortAndRepeatedCleanup"_el}) {
            const auto child = el::system::Subprocess::create(
                el::path::Path{el::unittest::fh::unitTestExecutablePath()},
                el::text::StringList{
                    el::String::fromJoined({"name:"_el, name}),
                    el::String::fromJoined({"+name:"_el, name}),
                    "--no-color"_el});
            child->start();
            const auto status = child->wait(el::time::TimeDelta::seconds(10));
            REQUIRE(status.has_value());
            REQUIRE_EQUAL(status->exitCode(), std::optional<std::int32_t>{0});
        }
    }

    SKIP_BY_DEFAULT()
    void testUdpAbortUnregistersBeforeCloseAndProtectsDescriptorReuse() {
        auto observer = RegistrationObserver{};
        auto device = PosixUdpSocketDevice::create(observer.function(), UdpSocketDeviceCallbacks{});
        const auto boundEndpoint = device->bind(endpoint(), ByteCount{256U});
        REQUIRE(boundEndpoint.port() != Port{});
        device->setReceiving(true);
        const auto descriptor = observer.lastDescriptor;
        const auto savedCallback = observer.savedCallback;
        device->abort();
        REQUIRE(observer.removedWhileOpen);
        REQUIRE_EQUAL(observer.removals, 1);
        REQUIRE_EQUAL(::fcntl(descriptor, F_GETFD), -1);
        device->abort();
        device->setReceiving(true);
        REQUIRE_EQUAL(observer.removals, 1);

        int pair[2]{};
        REQUIRE_EQUAL(::socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
        if (pair[1] == descriptor) {
            std::swap(pair[0], pair[1]);
        }
        if (pair[0] != descriptor) {
            REQUIRE_EQUAL(::dup2(pair[0], descriptor), descriptor);
            REQUIRE_EQUAL(::close(pair[0]), 0);
            pair[0] = descriptor;
        }
        auto ready = false;
        auto newRegistration = observer.driver->registerDescriptor(
            descriptor, true, false, [&ready](bool read, bool, bool) -> void { ready = read; });
        device.reset();
        savedCallback(true, true, true); // A previously copied closure must not access the destroyed device.
        constexpr char byte = 'x';
        REQUIRE_EQUAL(::write(pair[1], &byte, sizeof(byte)), static_cast<ssize_t>(sizeof(byte)));
        observer.driver->wait(el::time::TimeDelta::zero());
        REQUIRE(ready);
        newRegistration.reset();
        REQUIRE_EQUAL(::close(pair[0]), 0);
        REQUIRE_EQUAL(::close(pair[1]), 0);
    }

    SKIP_BY_DEFAULT()
    void testAbortWaitsForRegistrationCommit() {
        auto observer = RegistrationObserver{};
        const auto device = PosixUdpSocketDevice::create(observer.function(), UdpSocketDeviceCallbacks{});
        const auto boundEndpoint = device->bind(endpoint(), ByteCount{256U});
        REQUIRE(boundEndpoint.port() != Port{});
        auto abortStarted = std::atomic<bool>{false};
        auto worker = std::thread{};
        observer.duringRegistration = [&]() -> void {
            worker = std::thread{[&]() -> void {
                abortStarted.store(true);
                device->abort();
            }};
            while (!abortStarted.load()) {
                std::this_thread::yield();
            }
        };
        device->setReceiving(true);
        worker.join();
        REQUIRE(observer.removedWhileOpen);
        REQUIRE_EQUAL(observer.removals, 1);
        REQUIRE_EQUAL(::fcntl(observer.lastDescriptor, F_GETFD), -1);
        observer.savedCallback(true, true, true);
    }

    SKIP_BY_DEFAULT()
    void testTcpSynchronousCallbackCanReleaseSourceOwnership() {
        auto observer = RegistrationObserver{};
        auto device = PosixTcpConnectionDevicePtr{};
        auto callbackCalled = false;
        auto callbacks = TcpConnectionDeviceCallbacks{};
        callbacks.connected = [&](IpEndpoint, IpEndpoint) -> void {
            callbackCalled = true;
            device->abort();
            device.reset();
        };
        device = PosixTcpConnectionDevice::create(observer.function(), ByteCount{256U}, std::move(callbacks));
        const auto weak = device->weak_from_this();
        int pair[2]{};
        REQUIRE_EQUAL(::socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
        {
            const auto retained = device;
            retained->accept(std::make_unique<PosixTcpSocket>(pair[0], endpoint(), endpoint()));
            REQUIRE(callbackCalled);
            REQUIRE(device == nullptr);
            REQUIRE(!weak.expired());
            REQUIRE_EQUAL(::fcntl(pair[0], F_GETFD), -1);
        }
        REQUIRE(weak.expired());
        REQUIRE_EQUAL(::close(pair[1]), 0);
    }

    SKIP_BY_DEFAULT()
    void testTcpCallbackCanAbortAndReleaseItsOwnerFromAnotherThread() {
        auto observer = RegistrationObserver{};
        auto device = std::shared_ptr<PosixTcpConnectionDevice>{};
        auto callbackCalled = false;
        auto callbacks = TcpConnectionDeviceCallbacks{};
        callbacks.data = [&](el::mem::ByteBlock) -> void {
            callbackCalled = true;
            auto worker = std::thread{[&]() -> void { device->abort(); }};
            worker.join(); // Deadlocks if the native operation lock is held during notification.
            device.reset();
        };
        device = PosixTcpConnectionDevice::create(observer.function(), ByteCount{256U}, std::move(callbacks));
        const auto weak = device->weak_from_this();
        int pair[2]{};
        REQUIRE_EQUAL(::socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
        device->accept(std::make_unique<PosixTcpSocket>(pair[0], endpoint(), endpoint()));
        device->setReceiving(ByteCount{256U});
        constexpr char byte = 'x';
        REQUIRE_EQUAL(::write(pair[1], &byte, sizeof(byte)), static_cast<ssize_t>(sizeof(byte)));
        observer.driver->wait(el::time::TimeDelta::zero());
        REQUIRE(callbackCalled);
        REQUIRE(device == nullptr);
        REQUIRE(weak.expired());
        REQUIRE(observer.removedWhileOpen);
        observer.savedCallback(true, true, true);
        REQUIRE_EQUAL(::close(pair[1]), 0);
    }

    SKIP_BY_DEFAULT()
    void testListenerCrossThreadAbortAndRepeatedCleanup() {
        auto observer = RegistrationObserver{};
        auto device = PosixTcpListenerDevice::create(observer.function(), TcpListenerDeviceCallbacks{});
        const auto listeningEndpoint = device->start(endpoint(), ItemCount{4U});
        REQUIRE(listeningEndpoint.port() != Port{});
        device->setAccepting(true);
        auto worker = std::thread{[&]() -> void { device->abort(); }};
        worker.join();
        REQUIRE(observer.removedWhileOpen);
        REQUIRE_EQUAL(observer.removals, 1);
        device->close();
        device->setAccepting(true);
        device.reset();
        observer.savedCallback(true, false, false);
        REQUIRE_EQUAL(observer.removals, 1);
    }
};
