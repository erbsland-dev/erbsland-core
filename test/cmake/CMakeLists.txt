# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(resource-integration)

if(APPLE OR WIN32)
    add_test(
            NAME erbsland-core-package-install-integration
            COMMAND ${CMAKE_COMMAND}
                    -DPACKAGE_SOURCE_DIRECTORY=${PROJECT_SOURCE_DIR}
                    -DPACKAGE_TEST_DIRECTORY=${CMAKE_BINARY_DIR}/test/cmake/package-install-integration
                    -DPACKAGE_GENERATOR=${CMAKE_GENERATOR}
                    -DPACKAGE_CONFIGURATION=$<CONFIG>
                    -P ${PROJECT_SOURCE_DIR}/test/cmake/package-integration/RunPackageIntegration.cmake)
    set_tests_properties(erbsland-core-package-install-integration PROPERTIES RUN_SERIAL TRUE)
endif()

add_test(
        NAME erbsland-core-resource-installed-package
        COMMAND ${CMAKE_COMMAND}
                -DERBSLAND_CORE_BUILD_DIRECTORY=${CMAKE_BINARY_DIR}
                -DERBSLAND_CORE_FIXTURE_DIRECTORY=${PROJECT_SOURCE_DIR}/test/cmake/resource-installed-package
                -DERBSLAND_CORE_GENERATOR=${CMAKE_GENERATOR}
                -DERBSLAND_CORE_CONFIGURATION=$<CONFIG>
                -DERBSLAND_CORE_HOST_COMPILER_NAME=$<TARGET_FILE_NAME:erbsland-core-resource-compiler>
                -P ${PROJECT_SOURCE_DIR}/test/cmake/resource-installed-package/RunInstalledPackage.cmake
)
foreach(resource_error_case IN ITEMS
        invalid-target
        invalid-static-library-target
        invalid-resource-target
        export-static-library
        duplicate
        empty
        oversized
        identifier)
    add_test(
            NAME erbsland-core-resource-error-${resource_error_case}
            COMMAND ${CMAKE_COMMAND}
                    -S ${PROJECT_SOURCE_DIR}/test/cmake/resource-cmake-errors
                    -B ${CMAKE_BINARY_DIR}/test/cmake/resource-cmake-errors/${resource_error_case}
                    -DERBSLAND_CORE_SOURCE=${PROJECT_SOURCE_DIR}
                    -DRESOURCE_ERROR_CASE=${resource_error_case}
    )
    set_tests_properties(erbsland-core-resource-error-${resource_error_case} PROPERTIES WILL_FAIL TRUE)
endforeach()
