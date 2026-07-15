// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixErrorContext.hpp"

#include "../core/Definitions.hpp"
#include "../text/EscapeFormat.hpp"
#include "../text/Literals.hpp"
#include "../text/StringBuilder.hpp"
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

PosixErrorContext::PosixErrorContext(const ErrorCode errorCode, text::StringView errorMessage) noexcept :
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
        return std::make_shared<const PosixErrorContext>(errorCode, text::String{std::string_view{buffer.data()}});
    }
    return std::make_shared<const PosixErrorContext>(errorCode);
#else
    return std::make_shared<const PosixErrorContext>(errorCode, text::String{std::strerror(errorCode)});
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

auto PosixErrorContext::toString() const noexcept -> text::StringView {
    return _errorMessage;
}

auto PosixErrorContext::toTextDocument() const -> text::TextDocument {
    auto document = text::TextDocument{};
    auto list = document.add(text::TextNodeType::FieldList);
    auto code = list->add(text::TextNodeType::FieldItem);
    code->add(text::TextNodeType::FieldLabel)->addText("errno"_el);
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
