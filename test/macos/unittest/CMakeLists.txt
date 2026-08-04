# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

project(erbsland-core-macos-unittest LANGUAGES CXX)

if(NOT APPLE)
    message(FATAL_ERROR "The signed macOS unit test can only be built on macOS.")
endif()

if(NOT DEFINED CMAKE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON CACHE BOOL "Use unity builds for the standalone macOS unit test")
endif()

set(ERBSLAND_MACOS_DEVELOPMENT_TEAM "" CACHE STRING
        "Apple development team used for Xcode automatic signing")
set(ERBSLAND_MACOS_CODE_SIGN_IDENTITY "" CACHE STRING
        "Code-signing identity name or fingerprint; empty selects the first valid identity")
set(ERBSLAND_MACOS_BUNDLE_IDENTIFIER "dev.erbsland.core.macos-unittest")

cmake_path(SET ERBSLAND_CORE_ROOT NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../../..")

# Keep this manual test independent from every regular test, demo, fuzzing, and precompiled-header target.
set(ERBSLAND_CORE_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(ERBSLAND_CORE_ENABLE_DEMOS OFF CACHE BOOL "" FORCE)
set(ERBSLAND_CORE_ENABLE_FUZZING OFF CACHE BOOL "" FORCE)
set(ERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS OFF CACHE BOOL "" FORCE)
set(ERBSLAND_CORE_DEVELOPER_BUILD OFF CACHE BOOL "" FORCE)

add_subdirectory("${ERBSLAND_CORE_ROOT}" "${CMAKE_BINARY_DIR}/dependencies/erbsland-core" EXCLUDE_FROM_ALL)
add_subdirectory(
        "${ERBSLAND_CORE_ROOT}/test/erbsland-unittest"
        "${CMAKE_BINARY_DIR}/dependencies/erbsland-unittest"
        EXCLUDE_FROM_ALL
)

add_executable(erbsland-core-macos-unittest MACOSX_BUNDLE)
erbsland_set_required_compiler_options(erbsland-core-macos-unittest)
target_sources(erbsland-core-macos-unittest PRIVATE
        MacosProtectedDataTest.cpp
        main.cpp
)
target_link_libraries(erbsland-core-macos-unittest PRIVATE erbsland::core)
target_include_directories(erbsland-core-macos-unittest PRIVATE "${ERBSLAND_CORE_ROOT}/src")
set_target_properties(erbsland-core-macos-unittest PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Erbsland Core macOS Unit Test"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${ERBSLAND_MACOS_BUNDLE_IDENTIFIER}"
        OUTPUT_NAME "erbsland-core-macos-unittest"
        XCODE_GENERATE_SCHEME TRUE
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${ERBSLAND_MACOS_BUNDLE_IDENTIFIER}"
)

erbsland_unittest(TARGET erbsland-core-macos-unittest)

find_program(ERBSLAND_CODESIGN_EXECUTABLE codesign REQUIRED)
find_program(ERBSLAND_SECURITY_EXECUTABLE security REQUIRED)

set(_erbsland_entitlements_file "")
if(CMAKE_GENERATOR STREQUAL "Xcode" AND NOT ERBSLAND_MACOS_DEVELOPMENT_TEAM STREQUAL "")
    set(_erbsland_entitlements_file "${CMAKE_CURRENT_BINARY_DIR}/MacosProtectedData.entitlements")
    configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/MacosProtectedData.entitlements.in"
            "${_erbsland_entitlements_file}"
            @ONLY
    )
endif()

set(_erbsland_signing_identity "${ERBSLAND_MACOS_CODE_SIGN_IDENTITY}")
if(_erbsland_signing_identity STREQUAL "")
    execute_process(
            COMMAND "${ERBSLAND_SECURITY_EXECUTABLE}" find-identity -v -p codesigning
            RESULT_VARIABLE _erbsland_identity_result
            OUTPUT_VARIABLE _erbsland_identity_output
            ERROR_VARIABLE _erbsland_identity_error
    )
    if(_erbsland_identity_result EQUAL 0)
        string(REGEX MATCH "[0-9]+\\)[ \t]+([0-9A-Fa-f]+)[ \t]+\"" _erbsland_identity_match
                "${_erbsland_identity_output}")
        set(_erbsland_signing_identity "${CMAKE_MATCH_1}")
    endif()
endif()

if(CMAKE_GENERATOR STREQUAL "Xcode" AND NOT ERBSLAND_MACOS_DEVELOPMENT_TEAM STREQUAL "")
    set_target_properties(erbsland-core-macos-unittest PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
            XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${_erbsland_entitlements_file}"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
            XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${ERBSLAND_MACOS_DEVELOPMENT_TEAM}"
    )
    message(STATUS "Using Xcode automatic signing for the macOS unit test.")
elseif(CMAKE_GENERATOR STREQUAL "Xcode" AND NOT _erbsland_signing_identity STREQUAL "")
    set_target_properties(erbsland-core-macos-unittest PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${_erbsland_signing_identity}"
            XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
    )
    message(STATUS "Using the selected code-signing identity for the macOS unit test.")
else()
    if(NOT ERBSLAND_MACOS_DEVELOPMENT_TEAM STREQUAL "")
        message(WARNING
                "ERBSLAND_MACOS_DEVELOPMENT_TEAM only enables restricted entitlements with the Xcode generator, "
                "which can embed a matching development provisioning profile. The command-line generator will "
                "apply a plain signature without those entitlements."
        )
    endif()
    if(_erbsland_signing_identity STREQUAL "")
        set(_erbsland_signing_identity "-")
        message(WARNING
                "No valid code-signing identity was found. The test will be ad-hoc signed; Secure Enclave access may "
                "still fail. Use Xcode with ERBSLAND_MACOS_DEVELOPMENT_TEAM or set "
                "ERBSLAND_MACOS_CODE_SIGN_IDENTITY explicitly."
        )
    else()
        message(STATUS "Using the selected code-signing identity for the macOS unit test.")
    endif()
    add_custom_command(TARGET erbsland-core-macos-unittest POST_BUILD
            COMMAND "${ERBSLAND_CODESIGN_EXECUTABLE}"
                    --force
                    --identifier "${ERBSLAND_MACOS_BUNDLE_IDENTIFIER}"
                    --sign "${_erbsland_signing_identity}"
                    --timestamp=none
                    "$<TARGET_BUNDLE_DIR:erbsland-core-macos-unittest>"
            COMMENT "Code-signing the macOS protected-data unit test"
            VERBATIM
    )
endif()

add_custom_target(erbsland-core-macos-unittest-verify ALL
        COMMAND "${ERBSLAND_CODESIGN_EXECUTABLE}"
                --verify
                --deep
                --strict
                --verbose=2
                "$<TARGET_BUNDLE_DIR:erbsland-core-macos-unittest>"
        DEPENDS erbsland-core-macos-unittest
        COMMENT "Verifying the macOS protected-data unit-test signature"
        VERBATIM
)

enable_testing()
add_test(
        NAME erbsland-core-macos-unittest
        COMMAND "$<TARGET_FILE:erbsland-core-macos-unittest>" --no-color name:MacosProtectedData
)
