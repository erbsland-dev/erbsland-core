// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsErrorContext.hpp"

#include "../text/EscapeFormat.hpp"
#include "../text/Literals.hpp"
#include "../text/StringConverter.hpp"
#include "../text/TextDocument.hpp"
#include "../text/TextNode.hpp"
#include "../text/TextNodeType.hpp"

#include <memory>

namespace erbsland::system {

using namespace text::literals;

using namespace text;

WindowsErrorContext::WindowsErrorContext(const ErrorCode errorCode, String errorMessage) noexcept :
    _errorCode{errorCode}, _errorMessage{std::move(errorMessage)} {
}

auto WindowsErrorContext::messageFromErrorCode(const ErrorCode errorCode) -> String {
    wchar_t *buffer = nullptr;
    const auto length = ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr);
    auto bufferPtr = std::unique_ptr<void, decltype(&::LocalFree)>{buffer, &::LocalFree};
    if (length == 0U || buffer == nullptr) {
        return {};
    }
    return StringConverter{std::wstring_view{buffer, length}}.toString().trimmed();
}

auto WindowsErrorContext::fromErrorCode(const ErrorCode errorCode) -> std::shared_ptr<const WindowsErrorContext> {
    return std::make_shared<const WindowsErrorContext>(errorCode, messageFromErrorCode(errorCode));
}

auto WindowsErrorContext::fromLastError() -> std::shared_ptr<const WindowsErrorContext> {
    return fromErrorCode(::GetLastError());
}

auto WindowsErrorContext::category() const noexcept -> PlatformErrorCategory {
    switch (_errorCode) {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        return PlatformErrorCategory::NotFound;
    case ERROR_ACCESS_DENIED:
    case ERROR_PRIVILEGE_NOT_HELD:
        return PlatformErrorCategory::PermissionDenied;
    case ERROR_ALREADY_EXISTS:
    case ERROR_FILE_EXISTS:
        return PlatformErrorCategory::AlreadyExists;
    case ERROR_INVALID_NAME:
    case ERROR_BAD_PATHNAME:
    case ERROR_INVALID_DRIVE:
        return PlatformErrorCategory::InvalidPath;
    case ERROR_DIRECTORY:
        return PlatformErrorCategory::NotDirectory;
    case ERROR_FILENAME_EXCED_RANGE:
        return PlatformErrorCategory::NameTooLong;
    case ERROR_CANT_RESOLVE_FILENAME:
        return PlatformErrorCategory::SymbolicLinkLoop;
    case ERROR_WRITE_PROTECT:
        return PlatformErrorCategory::ReadOnlyFileSystem;
    case ERROR_DISK_FULL:
    case ERROR_HANDLE_DISK_FULL:
        return PlatformErrorCategory::StorageFull;
    case ERROR_DISK_QUOTA_EXCEEDED:
        return PlatformErrorCategory::QuotaExceeded;
    case ERROR_NOT_SAME_DEVICE:
        return PlatformErrorCategory::CrossDevice;
    case ERROR_BUSY:
    case ERROR_SHARING_VIOLATION:
    case ERROR_LOCK_VIOLATION:
        return PlatformErrorCategory::ResourceBusy;
    case ERROR_TOO_MANY_OPEN_FILES:
        return PlatformErrorCategory::TooManyOpenFiles;
    case ERROR_FILE_TOO_LARGE:
        return PlatformErrorCategory::FileTooLarge;
    case ERROR_NOT_SUPPORTED:
    case ERROR_CALL_NOT_IMPLEMENTED:
        return PlatformErrorCategory::Unsupported;
    default:
        return PlatformErrorCategory::Unknown;
    }
}

auto WindowsErrorContext::toString() const noexcept -> String {
    return _errorMessage;
}

auto WindowsErrorContext::toTextDocument() const -> TextDocument {
    auto document = TextDocument{};
    auto list = document.add(TextNodeType::FieldList);
    auto code = list->add(TextNodeType::FieldItem);
    code->add(TextNodeType::FieldLabel)->addText("Windows error code"_el);
    code->add(TextNodeType::FieldContent)->addText(String::fromInteger(_errorCode));
    if (!_errorMessage.isEmpty()) {
        auto message = list->add(TextNodeType::FieldItem);
        message->add(TextNodeType::FieldLabel)->addText("message"_el);
        message->add(TextNodeType::FieldContent)->addEscapedText(_errorMessage, EscapeFormat::Display);
    }
    return document;
}

}
