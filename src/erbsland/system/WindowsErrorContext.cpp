// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsErrorContext.hpp"

#include "../text/EscapeFormat.hpp"
#include "../text/Literals.hpp"
#include "../text/StringBuilder.hpp"
#include "../text/StringConverter.hpp"
#include "../text/TextDocument.hpp"
#include "../text/TextNode.hpp"
#include "../text/TextNodeType.hpp"

#include <memory>

namespace erbsland::system {

using namespace text::literals;

WindowsErrorContext::WindowsErrorContext(const ErrorCode errorCode, text::StringView errorMessage) noexcept :
    _errorCode{errorCode}, _errorMessage{std::move(errorMessage)} {
}

auto WindowsErrorContext::messageFromErrorCode(const ErrorCode errorCode) -> text::String {
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
    return text::StringConverter{std::wstring_view{buffer, length}}.toString().trim();
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

auto WindowsErrorContext::toString() const noexcept -> text::StringView {
    return _errorMessage;
}

auto WindowsErrorContext::toTextDocument() const -> text::TextDocument {
    auto document = text::TextDocument{};
    auto list = document.add(text::TextNodeType::FieldList);
    auto code = list->add(text::TextNodeType::FieldItem);
    code->add(text::TextNodeType::FieldLabel)->addText("Windows error code"_el);
    auto codeText = text::StringBuilder{};
    codeText.appendInteger(_errorCode);
    code->add(text::TextNodeType::FieldContent)->addText(codeText.toString());
    if (!_errorMessage.isEmpty()) {
        auto message = list->add(text::TextNodeType::FieldItem);
        message->add(text::TextNodeType::FieldLabel)->addText("message"_el);
        message->add(text::TextNodeType::FieldContent)->addEscapedText(_errorMessage, text::EscapeFormat::Display);
    }
    return document;
}

}
