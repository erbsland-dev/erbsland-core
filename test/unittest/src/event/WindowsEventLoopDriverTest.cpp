// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/impl/WindowsEventLoopDriver.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>

using namespace el::time;

TESTED_TARGETS(WindowsEventLoopDriver)
class WindowsEventLoopDriverTest final : public el::UnitTest {
public:
    void testHandleReadinessAndRegistration() {
        auto driver = el::event::impl::WindowsEventLoopDriver{};
        const auto pipeName =
            std::wstring{L"\\\\.\\pipe\\erbsland-event-loop-driver-"} + std::to_wstring(GetCurrentProcessId());
        const auto server = CreateNamedPipeW(
            pipeName.c_str(),
            PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1,
            64,
            64,
            0,
            nullptr);
        REQUIRE(server != INVALID_HANDLE_VALUE);
        auto called = false;
        auto completionError = DWORD{ERROR_INVALID_FUNCTION};
        auto overlapped = OVERLAPPED{};
        auto registration = driver.registerHandle(
            server,
            [&called, &completionError, &overlapped](
                [[maybe_unused]] DWORD transferred, OVERLAPPED *completed, const DWORD error) -> void {
                called = completed == &overlapped;
                completionError = error;
            });
        const auto connectResult = ConnectNamedPipe(server, &overlapped);
        REQUIRE(connectResult != 0 || GetLastError() == ERROR_IO_PENDING);
        const auto client =
            CreateFileW(pipeName.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        REQUIRE(client != INVALID_HANDLE_VALUE);

        driver.wait(TimeDelta{Seconds{1}});
        REQUIRE(called);
        REQUIRE_EQUAL(completionError, DWORD{ERROR_SUCCESS});

        registration.reset();
        REQUIRE(CloseHandle(client) != 0);
        REQUIRE(DisconnectNamedPipe(server) != 0);
        REQUIRE(CloseHandle(server) != 0);
    }
};
