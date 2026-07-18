// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixErrorContext.hpp"

#include "../core/Definitions.hpp"
#include "../text/EscapeFormat.hpp"
#include "../text/Literals.hpp"
#include "../text/StringConverter.hpp"
#include "../text/TextDocument.hpp"
#include "../text/TextNode.hpp"
#include "../text/TextNodeType.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <memory>
#include <string_view>

namespace erbsland::system {

using namespace text::literals;

using namespace text;

PosixErrorContext::PosixErrorContext(const ErrorCode errorCode, String errorMessage) noexcept :
    _errorCode{errorCode}, _errorMessage{std::move(errorMessage)} {
}

auto PosixErrorContext::fromErrno() -> std::shared_ptr<const PosixErrorContext> {
    const auto errorCode = errno;
    return fromErrorCode(errorCode);
}

auto PosixErrorContext::fromErrorCode(const ErrorCode errorCode) -> std::shared_ptr<const PosixErrorContext> {
#ifdef ERBSLAND_OS_WINDOWS
    auto buffer = std::array<char, 256U>{};
    if (::strerror_s(buffer.data(), buffer.size(), errorCode) == 0) {
        return std::make_shared<const PosixErrorContext>(
            errorCode, StringConverter{std::string_view{buffer.data()}}.toString());
    }
    return std::make_shared<const PosixErrorContext>(errorCode);
#else
    return std::make_shared<const PosixErrorContext>(
        errorCode, StringConverter{std::string_view{std::strerror(errorCode)}}.toString());
#endif
}

auto PosixErrorContext::category() const noexcept -> PlatformErrorCategory {
    switch (_errorCode) {
    case ENOENT:
        return PlatformErrorCategory::NotFound;
    case EACCES:
    case EPERM:
        return PlatformErrorCategory::PermissionDenied;
    case EEXIST:
        return PlatformErrorCategory::AlreadyExists;
    case EINVAL:
    case EBADF:
        return PlatformErrorCategory::InvalidPath;
    case ENOTDIR:
        return PlatformErrorCategory::NotDirectory;
    case EISDIR:
        return PlatformErrorCategory::IsDirectory;
    case ENAMETOOLONG:
        return PlatformErrorCategory::NameTooLong;
    case ELOOP:
        return PlatformErrorCategory::SymbolicLinkLoop;
    case EROFS:
        return PlatformErrorCategory::ReadOnlyFileSystem;
    case ENOSPC:
        return PlatformErrorCategory::StorageFull;
#ifdef EDQUOT
    case EDQUOT:
        return PlatformErrorCategory::QuotaExceeded;
#endif
    case EXDEV:
        return PlatformErrorCategory::CrossDevice;
    case EBUSY:
        return PlatformErrorCategory::ResourceBusy;
    case EMFILE:
    case ENFILE:
        return PlatformErrorCategory::TooManyOpenFiles;
    case EFBIG:
        return PlatformErrorCategory::FileTooLarge;
#ifdef ENOTSUP
    case ENOTSUP:
        return PlatformErrorCategory::Unsupported;
#endif
    default:
        return PlatformErrorCategory::Unknown;
    }
}

auto PosixErrorContext::toString() const noexcept -> String {
    return _errorMessage;
}

auto PosixErrorContext::toTextDocument() const -> TextDocument {
    auto document = TextDocument{};
    auto list = document.add(TextNodeType::FieldList);
    auto code = list->add(TextNodeType::FieldItem);
    code->add(TextNodeType::FieldLabel)->addText("errno"_el);
    code->add(TextNodeType::FieldContent)->addText(String::fromInteger(_errorCode));
    if (!_errorMessage.isEmpty()) {
        auto message = list->add(TextNodeType::FieldItem);
        message->add(TextNodeType::FieldLabel)->addText("message"_el);
        message->add(TextNodeType::FieldContent)->addEscapedText(_errorMessage, EscapeFormat::Display);
    }
    return document;
}

}
