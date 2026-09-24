# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

# Configure an Erbsland unit-test target with the project's build options.
#
# Usage:
#   erbsland_core_configure_unittest(<target> [<erbsland_unittest arguments>...])
#
# Parameters:
#   target - Name of the unit-test target to configure.
#
# Additional arguments are forwarded unchanged to erbsland_unittest(). Precompiled headers are enabled on the target
# and registered with the unit-test framework when ERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS is true.
function(erbsland_core_configure_unittest target)
    set(_options)
    if(ERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS)
        list(APPEND _options PRECOMPILE_HEADERS)
    endif()
    erbsland_enable_precompiled_headers(${target})
    erbsland_unittest(TARGET ${target} ${_options} ${ARGN})
endfunction()
