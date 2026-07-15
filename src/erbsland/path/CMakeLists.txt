# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        Path.cpp
        Path.hpp
        Path_fwd.hpp
        PathAccessInfo.hpp
        PathAccessProfile.hpp
        PathAccessRight.hpp
        PathAttribute.hpp
        PathChangeOptions.hpp
        PathCollisionMode.hpp
        PathContent.cpp
        PathContent.hpp
        PathContent_fwd.hpp
        PathCopyOptions.hpp
        PathCreateDirectoryOptions.hpp
        PathCreateFileOptions.hpp
        PathCreateMode.hpp
        PathError.cpp
        PathError.hpp
        PathErrorContext.cpp
        PathErrorContext.hpp
        PathFormat.hpp
        PathInfo.cpp
        PathInfo.hpp
        PathInfo_fwd.hpp
        PathInfoParts.hpp
        PathMoveOptions.hpp
        PathOperations.cpp
        PathOperations.hpp
        PathOperations_fwd.hpp
        PathProgress.hpp
        PathReadDataOptions.hpp
        PathReadTextOptions.cpp
        PathReadTextOptions.hpp
        PathRemoveOptions.hpp
        PathResolveMode.hpp
        PathResolveOptions.hpp
        PathTempDirectoryOptions.cpp
        PathTempDirectoryOptions.hpp
        PathTempFileOptions.cpp
        PathTempFileOptions.hpp
        PathType.hpp
        PathWalkDirection.hpp
        PathWalker.cpp
        PathWalker.hpp
        PathWalker_fwd.hpp
        PathWalkFn.hpp
        PathWalkOptions.hpp
        PathWalkResult.hpp
        PathWalkStatus.hpp
        PathWindowsFormat.hpp
        PathWriteDataOptions.hpp
        PathWriteTextOptions.cpp
        PathWriteTextOptions.hpp
        SymlinkMode.hpp
        TempDirectory.cpp
        TempDirectory.hpp
        TempDirectory_fwd.hpp
)
