# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

foreach(required IN ITEMS PACKAGE_SOURCE_DIRECTORY PACKAGE_TEST_DIRECTORY PACKAGE_GENERATOR PACKAGE_CONFIGURATION)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing package integration argument: ${required}")
    endif()
endforeach()

function(package_run)
    execute_process(COMMAND ${ARGN}
            RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Package integration command failed (${result}): ${ARGN}\n${output}\n${error}")
    endif()
endfunction()

function(package_assert_count directory expected)
    file(GLOB archives "${directory}/*.zip")
    list(LENGTH archives actual)
    if(NOT actual EQUAL expected)
        message(FATAL_ERROR "Expected ${expected} ZIPs in ${directory}, found ${actual}: ${archives}")
    endif()
endfunction()

set(fixture "${PACKAGE_SOURCE_DIRECTORY}/test/cmake/package-integration")
set(source_build "${PACKAGE_TEST_DIRECTORY}/source-build")
set(source_output "${PACKAGE_TEST_DIRECTORY}/source-packages")
set(installed_build "${PACKAGE_TEST_DIRECTORY}/installed-build")
set(installed_output "${PACKAGE_TEST_DIRECTORY}/installed-packages")
set(core_install "${PACKAGE_TEST_DIRECTORY}/core-install")
if(WIN32)
    set(executable_suffix ".exe")
else()
    set(executable_suffix "")
endif()
file(REMOVE_RECURSE "${PACKAGE_TEST_DIRECTORY}")

package_run("${CMAKE_COMMAND}" -S "${fixture}" -B "${source_build}" -G "${PACKAGE_GENERATOR}"
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_UNITY_BUILD=ON
        -DERBSLAND_CORE_ENABLE_TESTS=OFF -DERBSLAND_CORE_ENABLE_DEMOS=OFF
        "-DPACKAGE_CORE_SOURCE=${PACKAGE_SOURCE_DIRECTORY}"
        "-DPACKAGE_OUTPUT_DIRECTORY=${source_output}")
package_run("${CMAKE_COMMAND}" --build "${source_build}" --target package-client --config Release)
package_run("${CMAKE_COMMAND}" --install "${source_build}" --config Release --component Package-client)
package_assert_count("${source_output}" 1)

file(GLOB client_archives "${source_output}/client-*.zip")
list(LENGTH client_archives client_count)
if(NOT client_count EQUAL 1)
    message(FATAL_ERROR "Package-specific override did not select the client ZIP name.")
endif()
set(client_archive "${client_archives}")
set(extracted "${PACKAGE_TEST_DIRECTORY}/extracted")
if(APPLE)
    package_run(/usr/bin/ditto -x -k "${client_archive}" "${extracted}")
    file(GLOB clients "${extracted}/client-*/*.app/Contents/MacOS/package-client")
    file(GLOB libraries "${extracted}/client-*/*.app/Contents/Frameworks/libpackage-library.dylib")
    file(GLOB framework_binaries
            "${extracted}/client-*/*.app/Contents/Frameworks/PackageFramework.framework/Versions/A/PackageFramework")
    file(GLOB framework_links
            "${extracted}/client-*/*.app/Contents/Frameworks/PackageFramework.framework/Versions/Current")
    file(GLOB target_files "${extracted}/client-*/docs/target.txt")
    if(NOT libraries OR NOT framework_binaries OR NOT target_files OR NOT IS_SYMLINK "${framework_links}")
        message(FATAL_ERROR "The macOS package is missing its dylib, framework links, or target override file.")
    endif()
    package_run("${clients}")
elseif(WIN32)
    file(ARCHIVE_EXTRACT INPUT "${client_archive}" DESTINATION "${extracted}")
    file(GLOB clients "${extracted}/client-*/package-client.exe")
    file(GLOB libraries "${extracted}/client-*/package-library.dll")
    if(NOT libraries)
        message(FATAL_ERROR "The Windows package is missing its non-system DLL.")
    endif()
    package_run("${clients}")
endif()

file(REMOVE_RECURSE "${source_output}")
package_run("${CMAKE_COMMAND}" --install "${source_build}" --config Debug --component Package-client)
package_assert_count("${source_output}" 0)
package_run("${CMAKE_COMMAND}" --install "${source_build}" --config RelWithDebInfo --component Package-client)
package_assert_count("${source_output}" 1)
file(REMOVE_RECURSE "${source_output}")

if(WIN32)
    set(extra_targets package-signing-tool)
endif()
package_run("${CMAKE_COMMAND}" --build "${source_build}"
        --target package-server erbsland-core-resource-compiler ${extra_targets} --config Release)
package_run("${CMAKE_COMMAND}" --install "${source_build}" --config Release --prefix "${core_install}")
package_assert_count("${source_output}" 3)

set(package_tool "${source_build}/erbsland-core-package-tool-build/erbsland-core-package-tool${executable_suffix}")
set(version_output "${PACKAGE_TEST_DIRECTORY}/version-packages")
package_run("${package_tool}"
        --project-root "${fixture}" --project-name package-fixture --project-version 1.2.3
        --package-name versioned --config "${fixture}/package-version.elcl"
        --output-directory "${version_output}" --architecture testarch --cmake "${CMAKE_COMMAND}"
        --target-name package-server --target-path "${source_build}/package-server${executable_suffix}")
package_assert_count("${version_output}" 1)
set(version_archive "${version_output}/versioned-2.3.4-testarch.zip")
if(NOT EXISTS "${version_archive}")
    message(FATAL_ERROR "The version file or package placeholders were not applied.")
endif()
set(target_output "${PACKAGE_TEST_DIRECTORY}/target-packages")
package_run("${package_tool}"
        --project-root "${fixture}" --project-name package-fixture --project-version 1.2.3
        --package-name target-override --config "${fixture}/package-target.elcl"
        --output-directory "${target_output}" --architecture testarch --cmake "${CMAKE_COMMAND}"
        --target-name package-server --target-path "${source_build}/package-server${executable_suffix}")
if(NOT EXISTS "${target_output}/target-package-server-1.2.3.zip")
    message(FATAL_ERROR "The single-target filename override or target placeholder was not applied.")
endif()
set(version_extract "${PACKAGE_TEST_DIRECTORY}/version-extracted")
if(APPLE)
    package_run(/usr/bin/ditto -x -k "${version_archive}" "${version_extract}")
else()
    file(ARCHIVE_EXTRACT INPUT "${version_archive}" DESTINATION "${version_extract}")
endif()
if(NOT EXISTS "${version_extract}/versioned-2.3.4/assets/keep.txt" OR
        NOT EXISTS "${version_extract}/versioned-2.3.4/assets/nested/deep.txt" OR
        EXISTS "${version_extract}/versioned-2.3.4/assets/skip.txt")
    message(FATAL_ERROR "Additional-file include or exclude patterns selected the wrong files.")
endif()

find_program(package_git git REQUIRED)
set(git_root "${PACKAGE_TEST_DIRECTORY}/git-version")
# Git for Windows treats the Parallels shared directory as a different owner.
# Limit this exception to the temporary integration-test process.
if(WIN32)
    set(ENV{GIT_CONFIG_COUNT} 1)
    set(ENV{GIT_CONFIG_KEY_0} safe.directory)
    set(ENV{GIT_CONFIG_VALUE_0} "*")
endif()
file(MAKE_DIRECTORY "${git_root}")
file(WRITE "${git_root}/version.txt" "first\n")
package_run("${package_git}" -C "${git_root}" init -q)
package_run("${package_git}" -C "${git_root}" add version.txt)
package_run("${package_git}" -C "${git_root}" -c user.name=PackageTest
        -c user.email=package@example.invalid -c commit.gpgsign=false commit -qm first)
package_run("${package_git}" -C "${git_root}" -c tag.gpgsign=false tag v2.3.4)
file(APPEND "${git_root}/version.txt" "second\n")
package_run("${package_git}" -C "${git_root}" add version.txt)
package_run("${package_git}" -C "${git_root}" -c user.name=PackageTest
        -c user.email=package@example.invalid -c commit.gpgsign=false commit -qm second)
set(git_output "${PACKAGE_TEST_DIRECTORY}/git-packages")
package_run("${package_tool}"
        --project-root "${git_root}" --project-name package-fixture --project-version 1.2.3
        --package-name gitpack --config "${fixture}/package-git.elcl"
        --output-directory "${git_output}" --architecture testarch --cmake "${CMAKE_COMMAND}"
        --git "${package_git}" --target-name package-server
        --target-path "${source_build}/package-server${executable_suffix}")
file(GLOB git_archives "${git_output}/gitpack-2.3.4-1-*-testarch.zip")
list(LENGTH git_archives git_count)
if(NOT git_count EQUAL 1)
    message(FATAL_ERROR "Git version selection did not add commit distance and hash: ${git_archives}")
endif()
if(WIN32)
    unset(ENV{GIT_CONFIG_COUNT})
    unset(ENV{GIT_CONFIG_KEY_0})
    unset(ENV{GIT_CONFIG_VALUE_0})
endif()

if(WIN32)
    set(signing_output "${PACKAGE_TEST_DIRECTORY}/signing-packages")
    set(signing_log "${PACKAGE_TEST_DIRECTORY}/signing-commands.txt")
    set(ENV{PACKAGE_SIGN_TOOL} "${source_build}/package-signing-tool.exe")
    set(ENV{PACKAGE_SIGN_LOG} "${signing_log}")
    package_run("${package_tool}"
            --project-root "${fixture}" --project-name package-fixture --project-version 1.2.3
            --package-name signing --config "${fixture}/package-signing.elcl"
            --output-directory "${signing_output}" --architecture testarch --cmake "${CMAKE_COMMAND}"
            --target-name package-client --target-path "${source_build}/package-client.exe")
    package_assert_count("${signing_output}" 1)
    file(READ "${signing_log}" signing_commands)
    if(NOT signing_commands MATCHES "sign /fd sha256 /td sha256 /tr https://timestamp.example.invalid /sha1 0123456789ABCDEF0123456789ABCDEF01234567" OR
            NOT signing_commands MATCHES "verify /v /pa" OR
            NOT signing_commands MATCHES "package-library.dll" OR
            NOT signing_commands MATCHES "package-client.exe")
        message(FATAL_ERROR "Unexpected SignTool commands: ${signing_commands}")
    endif()
endif()

set(collision_output "${PACKAGE_TEST_DIRECTORY}/collision-packages")
execute_process(
        COMMAND "${package_tool}"
                --project-root "${fixture}" --project-name package-fixture --project-version 1.2.3
                --package-name collision --config "${fixture}/package-collision.elcl"
                --output-directory "${collision_output}" --architecture testarch --cmake "${CMAKE_COMMAND}"
                --target-name package-server --target-path "${source_build}/package-server${executable_suffix}"
        RESULT_VARIABLE collision_result OUTPUT_VARIABLE collision_stdout ERROR_VARIABLE collision_stderr)
if(collision_result EQUAL 0 OR NOT "${collision_stdout}${collision_stderr}" MATCHES "collision")
    message(FATAL_ERROR "A destination collision was not reported: ${collision_stdout}${collision_stderr}")
endif()
package_assert_count("${collision_output}" 0)
file(GLOB temporary_directories "${collision_output}/.erbsland-package-*")
if(temporary_directories)
    message(FATAL_ERROR "A failed package left temporary staging files: ${temporary_directories}")
endif()

file(REMOVE "${source_build}/package-server${executable_suffix}")
execute_process(
        COMMAND "${CMAKE_COMMAND}" --install "${source_build}" --config Release --component Package-server
        RESULT_VARIABLE missing_result OUTPUT_VARIABLE missing_output ERROR_VARIABLE missing_error)
if(missing_result EQUAL 0 OR NOT "${missing_output}${missing_error}" MATCHES "Build the required executable")
    message(FATAL_ERROR "A missing application build did not produce the build-first error: ${missing_output}${missing_error}")
endif()
package_assert_count("${source_output}" 3)

package_run("${CMAKE_COMMAND}" -S "${fixture}" -B "${installed_build}" -G "${PACKAGE_GENERATOR}"
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_UNITY_BUILD=ON
        -DPACKAGE_CORE_INSTALL=ON "-DCMAKE_PREFIX_PATH=${core_install}"
        "-DPACKAGE_OUTPUT_DIRECTORY=${installed_output}")
package_run("${CMAKE_COMMAND}" --build "${installed_build}" --target package-client --config Release)
package_run("${CMAKE_COMMAND}" --install "${installed_build}" --config Release --component Package-client)
package_assert_count("${installed_output}" 1)
