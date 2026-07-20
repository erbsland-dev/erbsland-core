// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamErrorContext.hpp"

#include "../system/PlatformErrorCategory.hpp"
#include "../system/PlatformErrorContext.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::stream {

using namespace text::literals;
using system::PlatformErrorCategory;
using system::PlatformErrorContextConstPtr;

StreamErrorContext::StreamErrorContext(text::String title, text::String description) noexcept :
    _title{std::move(title)}, _description{std::move(description)} {
}

auto StreamErrorContext::setTitle(text::String title) noexcept -> StreamErrorContext & {
    _title = std::move(title);
    return *this;
}

auto StreamErrorContext::setDescription(text::String description) noexcept -> StreamErrorContext & {
    _description = std::move(description);
    return *this;
}

auto StreamErrorContext::help() const noexcept -> text::String {
    return !_help.isEmpty() ? _help : categoryHelp();
}

auto StreamErrorContext::setHelp(text::String help) noexcept -> StreamErrorContext & {
    _help = std::move(help);
    return *this;
}

auto StreamErrorContext::setPath(text::String path) noexcept -> StreamErrorContext & {
    _path = std::move(path);
    return *this;
}

auto StreamErrorContext::setPlatformContext(PlatformErrorContextConstPtr platformContext) noexcept
    -> StreamErrorContext & {
    _platformContext = std::move(platformContext);
    return *this;
}

auto StreamErrorContext::categoryHelp() const noexcept -> text::String {
    if (_platformContext == nullptr) {
        return {};
    }
    switch (_platformContext->category().toRawValue()) {
    case PlatformErrorCategory::NotFound:
        return "Check that the stream path exists and is still accessible."_el;
    case PlatformErrorCategory::PermissionDenied:
        return "Check the stream permissions and access rights."_el;
    case PlatformErrorCategory::ReadOnlyFileSystem:
        return "Choose a writable stream target or change the filesystem settings."_el;
    case PlatformErrorCategory::StorageFull:
    case PlatformErrorCategory::QuotaExceeded:
        return "Free storage space or choose another stream target."_el;
    case PlatformErrorCategory::ResourceBusy:
        return "Close other users of the stream and try again."_el;
    case PlatformErrorCategory::TooManyOpenFiles:
        return "Close unused streams and try again."_el;
    case PlatformErrorCategory::FileTooLarge:
        return "Choose a target that supports the required stream size."_el;
    case PlatformErrorCategory::Unsupported:
        return "Choose a stream operation supported by this platform."_el;
    case PlatformErrorCategory::Unknown:
    case PlatformErrorCategory::AlreadyExists:
    case PlatformErrorCategory::InvalidPath:
    case PlatformErrorCategory::NotDirectory:
    case PlatformErrorCategory::IsDirectory:
    case PlatformErrorCategory::NameTooLong:
    case PlatformErrorCategory::SymbolicLinkLoop:
    case PlatformErrorCategory::CrossDevice:
    default:
        return {};
    }
}

}
