// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathErrorContext.hpp"

#include "../system/PlatformErrorCategory.hpp"
#include "../system/PlatformErrorContext.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::path {

using namespace text::literals;

PathErrorContext::PathErrorContext(text::StringView title, text::StringView description) noexcept :
    _title{std::move(title)}, _description{std::move(description)} {
}

auto PathErrorContext::setTitle(text::StringView title) noexcept -> PathErrorContext & {
    _title = std::move(title);
    return *this;
}

auto PathErrorContext::setDescription(text::StringView description) noexcept -> PathErrorContext & {
    _description = std::move(description);
    return *this;
}

auto PathErrorContext::help() const noexcept -> text::StringView {
    return !_help.isEmpty() ? _help : categoryHelp();
}

auto PathErrorContext::setHelp(text::StringView help) noexcept -> PathErrorContext & {
    _help = std::move(help);
    return *this;
}

auto PathErrorContext::setSourcePath(text::StringView sourcePath) noexcept -> PathErrorContext & {
    _sourcePath = std::move(sourcePath);
    return *this;
}

auto PathErrorContext::setTargetPath(text::StringView targetPath) noexcept -> PathErrorContext & {
    _targetPath = std::move(targetPath);
    return *this;
}

auto PathErrorContext::setPlatformContext(system::PlatformErrorContextConstPtr platformContext) noexcept
    -> PathErrorContext & {
    _platformContext = std::move(platformContext);
    return *this;
}

auto PathErrorContext::categoryHelp() const noexcept -> text::StringView {
    if (_platformContext == nullptr) {
        return {};
    }
    switch (_platformContext->category().toRawValue()) {
    case system::PlatformErrorCategory::NotFound:
        return "Check that the path is correct and that every parent directory exists."_el;
    case system::PlatformErrorCategory::PermissionDenied:
        return "Check the file permissions or choose a location you can access."_el;
    case system::PlatformErrorCategory::AlreadyExists:
        return "Choose another target or allow the existing item to be replaced."_el;
    case system::PlatformErrorCategory::InvalidPath:
        return "Correct the path and try the operation again."_el;
    case system::PlatformErrorCategory::NotDirectory:
        return "Check that every parent component in the path is a directory."_el;
    case system::PlatformErrorCategory::IsDirectory:
        return "Choose a file instead of a directory for this operation."_el;
    case system::PlatformErrorCategory::NameTooLong:
        return "Shorten the path or file name and try again."_el;
    case system::PlatformErrorCategory::SymbolicLinkLoop:
        return "Check the symbolic links in the path for a loop."_el;
    case system::PlatformErrorCategory::ReadOnlyFileSystem:
        return "Choose a writable location or change the filesystem settings."_el;
    case system::PlatformErrorCategory::StorageFull:
    case system::PlatformErrorCategory::QuotaExceeded:
        return "Free storage space or choose another filesystem."_el;
    case system::PlatformErrorCategory::CrossDevice:
        return "Copy the item to the target and remove the original afterwards."_el;
    case system::PlatformErrorCategory::ResourceBusy:
        return "Close applications using the path and try again."_el;
    case system::PlatformErrorCategory::TooManyOpenFiles:
        return "Close unused files and try again."_el;
    case system::PlatformErrorCategory::FileTooLarge:
        return "Choose a filesystem that supports a file of this size."_el;
    case system::PlatformErrorCategory::Unsupported:
        return "Choose an operation and filesystem supported on this platform."_el;
    case system::PlatformErrorCategory::Unknown:
    default:
        return {};
    }
}

}
