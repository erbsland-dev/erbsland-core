// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::path {

/// The type of the resource behind a path.
enum class PathType : std::uint8_t {
    Unknown = 0,             ///< Cannot be determined, does not exist, or unsupported type.
    Directory = 1U << 0U,    ///< Directory.
    RegularFile = 1U << 1U,  ///< Regular file.
    Symlink = 1U << 2U,      ///< Symbolic link to file or directory.
    Device = 1U << 3U,       ///< Character/block device, Windows device, volume, console, etc.
    Socket = 1U << 4U,       ///< Unix domain socket / Windows socket-like filesystem object if detectable.
    Pipe = 1U << 5U,         ///< FIFO / named pipe.
    ReparsePoint = 1U << 6U, ///< Windows reparse point that is not a symlink.
    All = Directory | RegularFile | Symlink | Device | Socket | Pipe | ReparsePoint, ///< All types.
};

using PathTypes = util::EnumFlags<PathType>;

}
