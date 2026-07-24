// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Path.hpp"
#include "../PathAccessInfo.hpp"
#include "../PathAttribute.hpp"
#include "../PathInfoParts.hpp"
#include "../PathType.hpp"

#include "../../system/GroupId.hpp"
#include "../../system/GroupName.hpp"
#include "../../system/UserId.hpp"
#include "../../system/UserName.hpp"
#include "../../text/StringEditor.hpp"
#include "../../time/DateTime.hpp"
#include "../../time/TimePoint.hpp"
#include "../../unit/ByteLength.hpp"

namespace erbsland::path::impl {

/// Shared data for path information.
/// @tested{PathInfoTest PosixPathInfoTest WindowsPathInfoTest}
class PathInfoData final {
public:
    PathInfoData() = default;

    // defaults
    ~PathInfoData() = default;
    PathInfoData(const PathInfoData &) = default;
    PathInfoData(PathInfoData &&) noexcept = default;
    auto operator=(const PathInfoData &) -> PathInfoData & = default;
    auto operator=(PathInfoData &&) noexcept -> PathInfoData & = default;

public:
    Path resolvedPath;                 ///< The resolved physical path.
    PathInfoParts requestedParts;      ///< The information parts requested for this instance.
    PathInfoParts loadedParts;         ///< The information parts loaded into this data.
    time::TimePoint lastRefresh;       ///< The time when the cached data was refreshed.
    bool exists{false};                ///< If the target exists.
    bool refreshFailed{false};         ///< If automatic refresh must stop until explicit reload.
    PathType type{PathType::Unknown};  ///< The target type.
    unit::ByteLength fileSize;         ///< Size for regular files.
    time::DateTime lastModified;       ///< Last modification time.
    time::DateTime lastAccessed;       ///< Last access time.
    time::DateTime birthTime;          ///< Creation/birth time when available.
    time::DateTime lastMetadataChange; ///< Last metadata change time when available.
    system::UserName ownerName;        ///< Resolved owner name.
    system::UserId ownerId;            ///< Platform owner identifier.
    system::GroupName groupName;       ///< Resolved group name.
    system::GroupId groupId;           ///< Platform group identifier.
    PathAccessInfo accessInfo;         ///< Portable access information.
    PathAttributes attributes;         ///< Native attributes.
};

}
