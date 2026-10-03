// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectTextFile.hpp"

#include <erbsland/PathInfo.hpp>
#include <erbsland/PathInfoParts.hpp>

#include <utility>

namespace tool::frame {

ProjectTextFile::ProjectTextFile(el::Path path) : _path{std::move(path)} {
    const auto info =
        _path.info(el::PathInfoParts{el::PathInfoPart::Size, el::PathInfoPart::Times, el::PathInfoPart::FileIdentity});
    _identity = info.fileIdentity();
    _size = info.fileSize();
    _modified = info.lastModified();
    _metadataChanged = info.lastMetadataChange();
}

auto ProjectTextFile::isCurrent() const -> bool {
    if (!_identity.isValid()) {
        return false;
    }
    auto info = _path.info();
    info.reload();
    return info.isRegularFile() && info.resolvedPath() == _path && info.fileIdentity() == _identity &&
        info.fileSize() == _size && info.lastModified() == _modified && info.lastMetadataChange() == _metadataChanged;
}

}
