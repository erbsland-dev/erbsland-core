// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathErrorContext.hpp"

#include "../system/PlatformErrorCategory.hpp"
#include "../system/PlatformErrorContext.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::path {

using namespace text::literals;

using system::PlatformErrorCategory;
using system::PlatformErrorContextConstPtr;

PathErrorContext::PathErrorContext(text::String title, text::String description) noexcept :
    _title{std::move(title)}, _description{std::move(description)} {
}

auto PathErrorContext::setTitle(text::String title) noexcept -> PathErrorContext & {
    _title = std::move(title);
    return *this;
}

auto PathErrorContext::setDescription(text::String description) noexcept -> PathErrorContext & {
    _description = std::move(description);
    return *this;
}

auto PathErrorContext::help() const noexcept -> text::String {
    return !_help.isEmpty() ? _help : categoryHelp();
}

auto PathErrorContext::setHelp(text::String help) noexcept -> PathErrorContext & {
    _help = std::move(help);
    return *this;
}

auto PathErrorContext::setSourcePath(text::String sourcePath) noexcept -> PathErrorContext & {
    _sourcePath = std::move(sourcePath);
    return *this;
}

auto PathErrorContext::setTargetPath(text::String targetPath) noexcept -> PathErrorContext & {
    _targetPath = std::move(targetPath);
    return *this;
}

auto PathErrorContext::setPlatformContext(PlatformErrorContextConstPtr platformContext) noexcept -> PathErrorContext & {
    _platformContext = std::move(platformContext);
    return *this;
}

auto PathErrorContext::categoryHelp() const noexcept -> text::String {
    if (_platformContext == nullptr) {
        return {};
    }
    switch (_platformContext->category().toRawValue()) {
    case PlatformErrorCategory::NotFound:
        return "Check that the path is correct and that every parent directory exists."_el;
    case PlatformErrorCategory::PermissionDenied:
        return "Check the file permissions or choose a location you can access."_el;
    case PlatformErrorCategory::AlreadyExists:
        return "Choose another target or allow the existing item to be replaced."_el;
    case PlatformErrorCategory::InvalidPath:
        return "Correct the path and try the operation again."_el;
    case PlatformErrorCategory::NotDirectory:
        return "Check that every parent component in the path is a directory."_el;
    case PlatformErrorCategory::IsDirectory:
        return "Choose a file instead of a directory for this operation."_el;
    case PlatformErrorCategory::NameTooLong:
        return "Shorten the path or file name and try again."_el;
    case PlatformErrorCategory::SymbolicLinkLoop:
        return "Check the symbolic links in the path for a loop."_el;
    case PlatformErrorCategory::ReadOnlyFileSystem:
        return "Choose a writable location or change the filesystem settings."_el;
    case PlatformErrorCategory::StorageFull:
    case PlatformErrorCategory::QuotaExceeded:
        return "Free storage space or choose another filesystem."_el;
    case PlatformErrorCategory::CrossDevice:
        return "Copy the item to the target and remove the original afterwards."_el;
    case PlatformErrorCategory::ResourceBusy:
        return "Close applications using the path and try again."_el;
    case PlatformErrorCategory::TooManyOpenFiles:
        return "Close unused files and try again."_el;
    case PlatformErrorCategory::FileTooLarge:
        return "Choose a filesystem that supports a file of this size."_el;
    case PlatformErrorCategory::Unsupported:
        return "Choose an operation and filesystem supported on this platform."_el;
    case PlatformErrorCategory::Unknown:
    default:
        return {};
    }
}

}
