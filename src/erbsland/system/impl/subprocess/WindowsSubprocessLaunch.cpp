// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsSubprocessLaunch.hpp"

#include "../WindowsErrorContext.hpp"

#include "../../../text/impl/PlatformU16StringAccess.hpp"
#include "../../../text/StringConverter.hpp"
#include "../../PlatformError.hpp"

#include <cwchar>
#include <memory>
#include <string_view>

namespace erbsland::system::impl {

using namespace text;
using namespace text::literals;

WindowsSubprocessLaunch::~WindowsSubprocessLaunch() {
    for (auto handle : {_outputRead, _outputWrite, _errorRead, _errorWrite, _nullHandle}) {
        if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
        }
    }
}

void WindowsSubprocessLaunch::prepareStreams(const SubprocessOptions &options) {
    const auto captureOutput = options.standardOutputMode() == SubprocessOutputMode::Capture;
    const auto captureError =
        !options.mergesStandardError() && options.standardErrorMode() == SubprocessOutputMode::Capture;
    auto security = SECURITY_ATTRIBUTES{};
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;
    if (captureOutput &&
        (!CreatePipe(&_outputRead, &_outputWrite, &security, 0U) ||
            !SetHandleInformation(_outputRead, HANDLE_FLAG_INHERIT, 0U))) {
        const auto error = WindowsErrorContext::fromLastError();
        throw PlatformError{"Failed to create the subprocess standard output pipe."_el, error};
    }
    if (captureError &&
        (!CreatePipe(&_errorRead, &_errorWrite, &security, 0U) ||
            !SetHandleInformation(_errorRead, HANDLE_FLAG_INHERIT, 0U))) {
        const auto error = WindowsErrorContext::fromLastError();
        throw PlatformError{"Failed to create the subprocess standard error pipe."_el, error};
    }

    const auto needsNull = !options.inheritsStandardInput() ||
        options.standardOutputMode() == SubprocessOutputMode::Discard ||
        (!options.mergesStandardError() && options.standardErrorMode() == SubprocessOutputMode::Discard);
    if (needsNull) {
        _nullHandle = CreateFileW(
            L"NUL",
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            &security,
            OPEN_EXISTING,
            0U,
            nullptr);
        if (_nullHandle == INVALID_HANDLE_VALUE) {
            const auto error = WindowsErrorContext::fromLastError();
            _nullHandle = nullptr;
            throw PlatformError{"Failed to open the null device for a subprocess."_el, error};
        }
    }
}

auto WindowsSubprocessLaunch::outputHandle(SubprocessOutputMode mode, HANDLE writer, DWORD inherited) const -> HANDLE {
    if (mode == SubprocessOutputMode::Capture) {
        return writer;
    }
    if (mode == SubprocessOutputMode::Discard) {
        return _nullHandle;
    }
    return GetStdHandle(inherited);
}

auto WindowsSubprocessLaunch::startupInfo(const SubprocessOptions &options) const -> STARTUPINFOW {
    auto startup = STARTUPINFOW{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = options.inheritsStandardInput() ? GetStdHandle(STD_INPUT_HANDLE) : _nullHandle;
    startup.hStdOutput = outputHandle(options.standardOutputMode(), _outputWrite, STD_OUTPUT_HANDLE);
    if (options.mergesStandardError()) {
        startup.hStdError = startup.hStdOutput;
    } else {
        startup.hStdError = outputHandle(options.standardErrorMode(), _errorWrite, STD_ERROR_HANDLE);
    }
    return startup;
}

auto WindowsSubprocessLaunch::commandLine(const std::wstring &nativeExecutablePath, const StringList &arguments)
    -> std::wstring {
    auto commandLine = quoteArgument(nativeExecutablePath);
    for (const auto &argument : arguments) {
        commandLine.push_back(L' ');
        commandLine.append(quoteArgument(StringConverter{argument}.toStdWString()));
    }

    return commandLine;
}

auto WindowsSubprocessLaunch::environmentBlock(const SubprocessOptions &options) -> std::vector<wchar_t> {
    // A null block asks Windows to inherit its environment without reconstruction.
    if (options.environmentChanges().isUnchangedInheritance()) {
        return {};
    }
    const auto environment = EnvironmentChangesProcessor::apply(options.environmentChanges(), inheritedEnvironment());
    return encodeEnvironment(environment);
}

auto WindowsSubprocessLaunch::inheritedEnvironment() -> Environment {
    const auto nativeEnvironment = GetEnvironmentStringsW();
    if (nativeEnvironment == nullptr) {
        throw PlatformError{"Failed to read the parent process environment."_el, WindowsErrorContext::fromLastError()};
    }
    const auto ownedEnvironment =
        std::unique_ptr<wchar_t, decltype(&FreeEnvironmentStringsW)>{nativeEnvironment, FreeEnvironmentStringsW};
    auto result = Environment{EnvironmentChangesProcessor::NameLess<U16String>{EnvironmentVariableBackend::native()}};
    for (auto cursor = nativeEnvironment; *cursor != L'\0'; cursor += std::wcslen(cursor) + 1U) {
        const auto entry = std::wstring_view{cursor};
        const auto separator = entry.find(L'=', entry.starts_with(L'=') ? 1U : 0U);
        if (separator != std::wstring_view::npos) {
            result.insert_or_assign(
                U16String{std::u16string_view{reinterpret_cast<const char16_t *>(cursor), separator}},
                U16String{std::u16string_view{
                    reinterpret_cast<const char16_t *>(cursor + separator + 1U), entry.size() - separator - 1U}});
        }
    }
    return result;
}

auto WindowsSubprocessLaunch::encodeEnvironment(const Environment &environment) -> std::vector<wchar_t> {
    auto block = std::vector<wchar_t>{};
    for (const auto &[name, value] : environment) {
        const auto nameAccess = text::impl::PlatformU16StringAccess{name};
        const auto valueAccess = text::impl::PlatformU16StringAccess{value};
        const auto *nameData = nameAccess.nullTerminatedWideCharPtr();
        const auto *valueData = valueAccess.nullTerminatedWideCharPtr();
        block.insert(block.end(), nameData, nameData + nameAccess.sizeWithoutNull());
        block.push_back(L'=');
        block.insert(block.end(), valueData, valueData + valueAccess.sizeWithoutNull());
        block.push_back(L'\0');
    }
    block.push_back(L'\0');
    if (environment.empty()) {
        block.push_back(L'\0');
    }
    return block;
}

auto WindowsSubprocessLaunch::start(
    const path::Path &executable, const StringList &arguments, const SubprocessOptions &options)
    -> PROCESS_INFORMATION {
    prepareStreams(options);
    auto startup = startupInfo(options);
    const auto nativeExecutablePath =
        StringConverter{executable.toWindows(path::PathWindowsFormat::Native)}.toStdWString();
    // Short native paths support children that cannot interpret the extended path prefix.
    const auto executablePath =
        nativeExecutablePath.size() < MAX_PATH ? nativeExecutablePath : executable.toStdPath().wstring();
    auto commandLine = WindowsSubprocessLaunch::commandLine(nativeExecutablePath, arguments);
    auto environmentBlock = WindowsSubprocessLaunch::environmentBlock(options);
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
        throw PlatformError{"Failed to launch the subprocess."_el, error};
    }
    CloseHandle(processInfo.hThread);
    closeWriters();
    return processInfo;
}

void WindowsSubprocessLaunch::closeWriters() noexcept {
    for (auto *handle : {&_outputWrite, &_errorWrite, &_nullHandle}) {
        if (*handle != nullptr) {
            CloseHandle(*handle);
            *handle = nullptr;
        }
    }
}

auto WindowsSubprocessLaunch::outputHandle() noexcept -> HANDLE & {
    return _outputRead;
}

auto WindowsSubprocessLaunch::errorHandle() noexcept -> HANDLE & {
    return _errorRead;
}

auto WindowsSubprocessLaunch::quoteArgument(const std::wstring &argument) -> std::wstring {
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

}
