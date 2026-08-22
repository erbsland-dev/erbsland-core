# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

install(TARGETS ${_erbsland_core_install_targets}
        EXPORT erbsland-core-targets
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(DIRECTORY include/
        DESTINATION include
)
install(DIRECTORY src/erbsland/
        DESTINATION src/erbsland
        FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.tpp"
)
install(EXPORT erbsland-core-targets
        FILE erbsland-core-targets.cmake
        NAMESPACE ErbslandDEV::
        DESTINATION lib/cmake/erbsland-core
)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/erbsland-coreConfigVersion.cmake"
        VERSION 1.0.0
        COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
        "${CMAKE_CURRENT_LIST_DIR}/erbsland-coreConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/erbsland-coreConfig.cmake"
        INSTALL_DESTINATION lib/cmake/erbsland-core
)
install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/git-version.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/application.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/resources.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/erbsland-coreConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/erbsland-coreConfigVersion.cmake"
        DESTINATION lib/cmake/erbsland-core
)
