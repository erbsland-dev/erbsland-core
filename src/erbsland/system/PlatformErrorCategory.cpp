// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PlatformErrorCategory.hpp"

#include "../text/Literals.hpp"
#include "../text/String.hpp"
#include "../text/StringView.hpp"

namespace erbsland::system {

using namespace text::literals;

auto PlatformErrorCategory::toString() const -> text::StringView {
    switch (_value) {
    case NotFound:
        return "not-found"_el;
    case PermissionDenied:
        return "permission-denied"_el;
    case AlreadyExists:
        return "already-exists"_el;
    case InvalidPath:
        return "invalid-path"_el;
    case NotDirectory:
        return "not-directory"_el;
    case IsDirectory:
        return "is-directory"_el;
    case NameTooLong:
        return "name-too-long"_el;
    case SymbolicLinkLoop:
        return "symbolic-link-loop"_el;
    case ReadOnlyFileSystem:
        return "read-only-file-system"_el;
    case StorageFull:
        return "storage-full"_el;
    case QuotaExceeded:
        return "quota-exceeded"_el;
    case CrossDevice:
        return "cross-device"_el;
    case ResourceBusy:
        return "resource-busy"_el;
    case TooManyOpenFiles:
        return "too-many-open-files"_el;
    case FileTooLarge:
        return "file-too-large"_el;
    case Unsupported:
        return "unsupported"_el;
    case Unknown:
    default:
        return "unknown"_el;
    }
}

}
