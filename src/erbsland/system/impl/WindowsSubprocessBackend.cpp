// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsSubprocessBackend.hpp"

#include "ProcessIdAccess.hpp"
#include "WindowsErrorContext.hpp"

#include "../PlatformError.hpp"
#include "../SubprocessOptions.hpp"

#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"

#include <algorithm>
#include <array>
#include <cwchar>
#include <map>
#include <string_view>
#include <vector>

namespace erbsland::system::impl {
using namespace text::literals;

WindowsSubprocessBackend::WindowsSubprocessBackend(
    const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options) {
    auto outputRead = HANDLE{};
    auto outputWrite = HANDLE{};
    auto errorRead = HANDLE{};
    auto errorWrite = HANDLE{};
    auto nullHandle = HANDLE{};
    const auto captureOutput = options.standardOutputMode() == SubprocessOutputMode::Capture;
    const auto captureError =
        !options.mergesStandardError() && options.standardErrorMode() == SubprocessOutputMode::Capture;
    auto security = SECURITY_ATTRIBUTES{};
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;
    const auto closeHandles = [&]() noexcept -> void {
        for (auto handle : {outputRead, outputWrite, errorRead, errorWrite, nullHandle}) {
            if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
                CloseHandle(handle);
            }
        }
    };
    if (captureOutput &&
        (!CreatePipe(&outputRead, &outputWrite, &security, 0U) ||
            !SetHandleInformation(outputRead, HANDLE_FLAG_INHERIT, 0U))) {
        const auto error = WindowsErrorContext::fromLastError();
        closeHandles();
        throw PlatformError{"Failed to create the subprocess standard output pipe."_el, error};
    }
    if (captureError &&
        (!CreatePipe(&errorRead, &errorWrite, &security, 0U) ||
            !SetHandleInformation(errorRead, HANDLE_FLAG_INHERIT, 0U))) {
        const auto error = WindowsErrorContext::fromLastError();
        closeHandles();
        throw PlatformError{"Failed to create the subprocess standard error pipe."_el, error};
    }

    const auto needsNull = !options.inheritsStandardInput() ||
        options.standardOutputMode() == SubprocessOutputMode::Discard ||
        (!options.mergesStandardError() && options.standardErrorMode() == SubprocessOutputMode::Discard);
    if (needsNull) {
        nullHandle = CreateFileW(
            L"NUL",
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            &security,
            OPEN_EXISTING,
            0U,
            nullptr);
        if (nullHandle == INVALID_HANDLE_VALUE) {
            const auto error = WindowsErrorContext::fromLastError();
            nullHandle = nullptr;
            closeHandles();
            throw PlatformError{"Failed to open the null device for a subprocess."_el, error};
        }
    }

    auto startup = STARTUPINFOW{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = options.inheritsStandardInput() ? GetStdHandle(STD_INPUT_HANDLE) : nullHandle;
    startup.hStdOutput = captureOutput                                  ? outputWrite
        : options.standardOutputMode() == SubprocessOutputMode::Discard ? nullHandle
                                                                        : GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = options.mergesStandardError()                  ? startup.hStdOutput
        : captureError                                                 ? errorWrite
        : options.standardErrorMode() == SubprocessOutputMode::Discard ? nullHandle
                                                                       : GetStdHandle(STD_ERROR_HANDLE);

    const auto executablePath = executable.toStdPath().wstring();
    auto commandLine = quoteArgument(executablePath);
    for (const auto &argument : arguments) {
        commandLine.push_back(L' ');
        commandLine.append(quoteArgument(text::StringConverter{argument}.toStdWString()));
    }

    struct CaseInsensitiveLess final {
        auto operator()(const std::wstring &first, const std::wstring &second) const noexcept -> bool {
            return _wcsicmp(first.c_str(), second.c_str()) < 0;
        }
    };
    auto environment = std::map<std::wstring, std::wstring, CaseInsensitiveLess>{};
    if (options.inheritsEnvironment()) {
        auto nativeEnvironment = GetEnvironmentStringsW();
        if (nativeEnvironment == nullptr) {
            const auto error = WindowsErrorContext::fromLastError();
            closeHandles();
            throw PlatformError{"Failed to read the parent process environment."_el, error};
        }
        for (auto cursor = nativeEnvironment; *cursor != L'\0'; cursor += std::wcslen(cursor) + 1U) {
            const auto entry = std::wstring_view{cursor};
            const auto separator = entry.find(L'=', entry.starts_with(L'=') ? 1U : 0U);
            if (separator != std::wstring_view::npos) {
                environment.insert_or_assign(
                    std::wstring{entry.substr(0U, separator)}, std::wstring{entry.substr(separator + 1U)});
            }
        }
        FreeEnvironmentStringsW(nativeEnvironment);
    }
    for (const auto &[name, value] : options.environmentChanges()) {
        const auto nativeName = text::StringConverter{name}.toStdWString();
        if (value.has_value()) {
            environment.insert_or_assign(nativeName, text::StringConverter{*value}.toStdWString());
        } else {
            environment.erase(nativeName);
        }
    }
    auto environmentBlock = std::vector<wchar_t>{};
    if (!options.inheritsEnvironment() || !options.environmentChanges().count().isZero()) {
        for (const auto &[name, value] : environment) {
            environmentBlock.insert(environmentBlock.end(), name.begin(), name.end());
            environmentBlock.push_back(L'=');
            environmentBlock.insert(environmentBlock.end(), value.begin(), value.end());
            environmentBlock.push_back(L'\0');
        }
        environmentBlock.push_back(L'\0');
        if (environment.empty()) {
            environmentBlock.push_back(L'\0');
        }
    }
    const auto workingDirectory =
        options.workingDirectory().has_value() ? options.workingDirectory()->toStdPath().wstring() : std::wstring{};
    auto processInfo = PROCESS_INFORMATION{};
    if (!CreateProcessW(
            executablePath.c_str(),
            commandLine.data(),
            nullptr,
            nullptr,
            TRUE,
            CREATE_UNICODE_ENVIRONMENT,
            environmentBlock.empty() ? nullptr : environmentBlock.data(),
            workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
            &startup,
            &processInfo)) {
        const auto error = WindowsErrorContext::fromLastError();
        closeHandles();
        throw PlatformError{"Failed to launch the subprocess."_el, error};
    }
    CloseHandle(processInfo.hThread);
    _process = processInfo.hProcess;
    _processId = processInfo.dwProcessId;
    if (outputWrite != nullptr) {
        CloseHandle(outputWrite);
        outputWrite = nullptr;
    }
    if (errorWrite != nullptr) {
        CloseHandle(errorWrite);
        errorWrite = nullptr;
    }
    if (nullHandle != nullptr) {
        CloseHandle(nullHandle);
        nullHandle = nullptr;
    }
    if (captureOutput) {
        _standardOutput = std::make_shared<CaptureState>();
        _standardOutput->limit = options.captureLimit().toSizeT();
        _standardOutputReader = std::thread{&WindowsSubprocessBackend::readCaptured, outputRead, _standardOutput};
        outputRead = nullptr;
    }
    if (captureError) {
        _standardError = std::make_shared<CaptureState>();
        _standardError->limit = options.captureLimit().toSizeT();
        _standardErrorReader = std::thread{&WindowsSubprocessBackend::readCaptured, errorRead, _standardError};
        errorRead = nullptr;
    }
    closeHandles();
}

WindowsSubprocessBackend::~WindowsSubprocessBackend() {
    joinReaders();
    if (_process != nullptr) {
        CloseHandle(_process);
    }
}

auto WindowsSubprocessBackend::processId() const noexcept -> ProcessId {
    return ProcessIdAccess::fromNative(static_cast<std::uint64_t>(_processId));
}

auto WindowsSubprocessBackend::isRunning() -> bool {
    const auto lock = std::scoped_lock{_waitMutex};
    if (_exitStatus.has_value()) {
        return false;
    }
    const auto result = WaitForSingleObject(_process, 0U);
    if (result == WAIT_TIMEOUT) {
        return true;
    }
    if (result != WAIT_OBJECT_0) {
        throw PlatformError{"Failed while polling the subprocess."_el, WindowsErrorContext::fromLastError()};
    }
    storeStatus();
    return false;
}

auto WindowsSubprocessBackend::exitStatus() const noexcept -> const std::optional<SubprocessExitStatus> & {
    return _exitStatus;
}

auto WindowsSubprocessBackend::wait() -> SubprocessExitStatus {
    {
        const auto lock = std::scoped_lock{_waitMutex};
        if (!_exitStatus.has_value()) {
            const auto result = WaitForSingleObject(_process, INFINITE);
            if (result != WAIT_OBJECT_0) {
                throw PlatformError{
                    "Failed while waiting for the subprocess."_el, WindowsErrorContext::fromLastError()};
            }
            storeStatus();
        }
    }
    joinReaders();
    return *_exitStatus;
}

auto WindowsSubprocessBackend::wait(const time::TimeDelta timeout) -> std::optional<SubprocessExitStatus> {
    const auto milliseconds = timeout.toMilliseconds().toRawValue();
    const auto nativeTimeout = static_cast<DWORD>(std::min<std::int64_t>(milliseconds, MAXDWORD - 1U));
    {
        const auto lock = std::scoped_lock{_waitMutex};
        if (!_exitStatus.has_value()) {
            const auto result = WaitForSingleObject(_process, nativeTimeout);
            if (result == WAIT_TIMEOUT) {
                return std::nullopt;
            }
            if (result != WAIT_OBJECT_0) {
                throw PlatformError{
                    "Failed while waiting for the subprocess."_el, WindowsErrorContext::fromLastError()};
            }
            storeStatus();
        }
    }
    joinReaders();
    return _exitStatus;
}

void WindowsSubprocessBackend::terminate() {
    if (_exitStatus.has_value() || TerminateProcess(_process, 1U)) {
        return;
    }
    throw PlatformError{"Failed to terminate the subprocess."_el, WindowsErrorContext::fromLastError()};
}

void WindowsSubprocessBackend::kill() {
    terminate();
}

auto WindowsSubprocessBackend::standardOutput() const -> text::String {
    return capturedText(_standardOutput);
}

auto WindowsSubprocessBackend::standardError() const -> text::String {
    return capturedText(_standardError);
}

auto WindowsSubprocessBackend::wasStandardOutputTruncated() const noexcept -> bool {
    return wasTruncated(_standardOutput);
}

auto WindowsSubprocessBackend::wasStandardErrorTruncated() const noexcept -> bool {
    return wasTruncated(_standardError);
}

void WindowsSubprocessBackend::storeStatus() {
    auto exitCode = DWORD{};
    if (!GetExitCodeProcess(_process, &exitCode)) {
        throw PlatformError{"Failed to read the subprocess exit code."_el, WindowsErrorContext::fromLastError()};
    }
    _exitStatus = SubprocessExitStatus::exited(static_cast<std::int32_t>(exitCode));
}

void WindowsSubprocessBackend::joinReaders() {
    if (_standardOutputReader.joinable()) {
        _standardOutputReader.join();
    }
    if (_standardErrorReader.joinable()) {
        _standardErrorReader.join();
    }
}

void WindowsSubprocessBackend::readCaptured(const HANDLE handle, const std::shared_ptr<CaptureState> &state) noexcept {
    auto buffer = std::array<char, 8192>{};
    while (true) {
        auto count = DWORD{};
        if (!ReadFile(handle, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr) || count == 0U) {
            break;
        }
        const auto lock = std::scoped_lock{state->mutex};
        const auto available = state->limit > state->bytes.size() ? state->limit - state->bytes.size() : 0U;
        const auto retain = std::min<std::size_t>(available, count);
        state->bytes.append(buffer.data(), retain);
        state->truncated = state->truncated || retain < count;
    }
    CloseHandle(handle);
}

auto WindowsSubprocessBackend::capturedText(const std::shared_ptr<CaptureState> &state) -> text::String {
    if (state == nullptr) {
        return {};
    }
    const auto lock = std::scoped_lock{state->mutex};
    return text::String{std::string_view{state->bytes}};
}

auto WindowsSubprocessBackend::wasTruncated(const std::shared_ptr<CaptureState> &state) noexcept -> bool {
    if (state == nullptr) {
        return false;
    }
    const auto lock = std::scoped_lock{state->mutex};
    return state->truncated;
}

auto WindowsSubprocessBackend::quoteArgument(const std::wstring &argument) -> std::wstring {
    if (!argument.empty() && argument.find_first_of(L" \t\"") == std::wstring::npos) {
        return argument;
    }
    auto result = std::wstring{L"\""};
    auto backslashes = std::size_t{};
    for (const auto character : argument) {
        if (character == L'\\') {
            ++backslashes;
            continue;
        }
        if (character == L'\"') {
            result.append(backslashes * 2U + 1U, L'\\');
            result.push_back(L'\"');
        } else {
            result.append(backslashes, L'\\');
            result.push_back(character);
        }
        backslashes = 0U;
    }
    result.append(backslashes * 2U, L'\\');
    result.push_back(L'\"');
    return result;
}

auto createSubprocessBackend(
    const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options)
    -> SubprocessBackendPtr {
    return std::make_unique<WindowsSubprocessBackend>(executable, arguments, options);
}

}
