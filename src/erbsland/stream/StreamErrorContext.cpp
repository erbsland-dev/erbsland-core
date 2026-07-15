// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamErrorContext.hpp"

#include "../system/PlatformErrorCategory.hpp"
#include "../system/PlatformErrorContext.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::stream {

using namespace text::literals;

StreamErrorContext::StreamErrorContext(text::StringView title, text::StringView description) noexcept :
    _title{std::move(title)}, _description{std::move(description)} {
}

auto StreamErrorContext::setTitle(text::StringView title) noexcept -> StreamErrorContext & {
    _title = std::move(title);
    return *this;
}

auto StreamErrorContext::setDescription(text::StringView description) noexcept -> StreamErrorContext & {
    _description = std::move(description);
    return *this;
}

auto StreamErrorContext::help() const noexcept -> text::StringView {
    return !_help.isEmpty() ? _help : categoryHelp();
}

auto StreamErrorContext::setHelp(text::StringView help) noexcept -> StreamErrorContext & {
    _help = std::move(help);
    return *this;
}

auto StreamErrorContext::setPath(text::StringView path) noexcept -> StreamErrorContext & {
    _path = std::move(path);
    return *this;
}

auto StreamErrorContext::setPlatformContext(system::PlatformErrorContextConstPtr platformContext) noexcept
    -> StreamErrorContext & {
    _platformContext = std::move(platformContext);
    return *this;
}

auto StreamErrorContext::categoryHelp() const noexcept -> text::StringView {
    if (_platformContext == nullptr) {
        return {};
    }
    switch (_platformContext->category().toRawValue()) {
    case system::PlatformErrorCategory::NotFound:
        return "Check that the stream path exists and is still accessible."_el;
    case system::PlatformErrorCategory::PermissionDenied:
        return "Check the stream permissions and access rights."_el;
    case system::PlatformErrorCategory::ReadOnlyFileSystem:
        return "Choose a writable stream target or change the filesystem settings."_el;
    case system::PlatformErrorCategory::StorageFull:
    case system::PlatformErrorCategory::QuotaExceeded:
        return "Free storage space or choose another stream target."_el;
    case system::PlatformErrorCategory::ResourceBusy:
        return "Close other users of the stream and try again."_el;
    case system::PlatformErrorCategory::TooManyOpenFiles:
        return "Close unused streams and try again."_el;
    case system::PlatformErrorCategory::FileTooLarge:
        return "Choose a target that supports the required stream size."_el;
    case system::PlatformErrorCategory::Unsupported:
        return "Choose a stream operation supported by this platform."_el;
    case system::PlatformErrorCategory::Unknown:
    case system::PlatformErrorCategory::AlreadyExists:
    case system::PlatformErrorCategory::InvalidPath:
    case system::PlatformErrorCategory::NotDirectory:
    case system::PlatformErrorCategory::IsDirectory:
    case system::PlatformErrorCategory::NameTooLong:
    case system::PlatformErrorCategory::SymbolicLinkLoop:
    case system::PlatformErrorCategory::CrossDevice:
    default:
        return {};
    }
}

}
