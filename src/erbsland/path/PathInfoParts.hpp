// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::path {

/// The part of path information to request and cache.
enum class PathInfoPart : uint16_t {
    None = 0U,                   ///< No parts requested.
    Type = 1U << 0U,             ///< The type of the path and if it exists. Also resolving the path.
    Size = 1U << 1U,             ///< The size of the path when it is a regular file.
    Times = 1U << 2U,            ///< All time information.
    OwnerId = 1U << 3U,          ///< The owner identifier.
    OwnerName = 1U << 4U,        ///< The resolved owner name.
    GroupId = 1U << 5U,          ///< The group identifier.
    GroupName = 1U << 6U,        ///< The resolved group name.
    AccessRights = 1U << 7U,     ///< Portable access rights.
    Attributes = 1U << 8U,       ///< Native attributes.
    FileIdentity = 1U << 9U,     ///< Stable identity of the current filesystem object.

    Owner = OwnerId | OwnerName, ///< The owner identifier and name.
    Group = GroupId | GroupName, ///< The group identifier and name.
    Identity = Owner | Group,    ///< Owner and group identifiers and names.
    Default = Type,              ///< The default parts to request.
    All = Type | Size | Times | Identity | AccessRights | Attributes | FileIdentity, ///< All parts requested.
};

/// The parts of path information to initially request and cache.
using PathInfoParts = util::EnumFlags<PathInfoPart>;

}
