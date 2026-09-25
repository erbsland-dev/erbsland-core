# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-package-tool EXCLUDE_FROM_ALL
        src/FileEntry.hpp
        src/PackageApplication.cpp
        src/PackageApplication.hpp
        src/PackageConfig.cpp
        src/PackageConfig.hpp
        src/PackageFiles.cpp
        src/PackageFiles.hpp
        src/PackagePlatform.cpp
        src/PackagePlatform.hpp
        src/PackageSettings.hpp
        src/PackageTarget.hpp
        src/impl/PackagePlaceholderSource.cpp
        src/impl/PackagePlaceholderSource.hpp
        src/main.cpp
)
target_compile_definitions(erbsland-core-package-tool PRIVATE
        ERBSLAND_CORE_PACKAGE_SCRIPT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
if(TARGET erbsland-core)
    target_link_libraries(erbsland-core-package-tool PRIVATE erbsland-core)
    target_include_directories(erbsland-core-package-tool PRIVATE "${PROJECT_SOURCE_DIR}/src")
    erbsland_set_required_compiler_options(erbsland-core-package-tool)
    erbsland_enable_debug_warnings(erbsland-core-package-tool)
else()
    target_link_libraries(erbsland-core-package-tool PRIVATE ErbslandDEV::erbsland-core)
    target_compile_features(erbsland-core-package-tool PRIVATE cxx_std_20)
endif()
