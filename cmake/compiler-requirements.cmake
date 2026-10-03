# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.23)
include_guard()

# Check the compiler for both source builds and installed-package consumers.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND MSVC_VERSION LESS 1950)
    message(FATAL_ERROR "Erbsland Core requires MSVC 19.50 or later.")
endif()
