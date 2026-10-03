# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
project(display-text-consumer LANGUAGES CXX)
find_package(erbsland-core CONFIG REQUIRED)

add_executable(display-text-consumer)
target_link_libraries(display-text-consumer PRIVATE ErbslandDEV::erbsland-core)
target_compile_features(display-text-consumer PRIVATE cxx_std_20)
set_target_properties(display-text-consumer PROPERTIES UNITY_BUILD ON)
set(catalog_class Labels)
set(catalog_namespace sample::wording)
if(DISPLAY_TEXT_RENAMED)
    set(catalog_class RenamedLabels)
    set(catalog_namespace sample::renamed)
endif()
if(DISPLAY_TEXT_CASE STREQUAL "cross-missing")
    set(CMAKE_CROSSCOMPILING TRUE)
elseif(DISPLAY_TEXT_CASE STREQUAL "cross-explicit")
    set(CMAKE_CROSSCOMPILING TRUE)
    set(ERBSLAND_CORE_DISPLAY_TEXT_COMPILER_EXECUTABLE "${HOST_COMPILER}")
elseif(DISPLAY_TEXT_CASE STREQUAL "missing-host")
    set(ERBSLAND_CORE_DISPLAY_TEXT_COMPILER_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/missing-compiler")
elseif(DISPLAY_TEXT_CASE STREQUAL "outside-build")
    set(extra_arguments HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Outside.hpp")
elseif(DISPLAY_TEXT_CASE STREQUAL "invalid-class")
    set(extra_arguments CLASS "not-valid")
endif()
erbsland_core_display_text(
        TARGET display-text-consumer
        FILE Words.elcl
        HEADER generated/include/Labels.hpp
        SOURCE generated/source/Labels.cpp
        CLASS "${catalog_class}"
        NAMESPACE "${catalog_namespace}"
        ${extra_arguments}
)
erbsland_core_display_text(TARGET display-text-consumer FILE Extra.elcl)
erbsland_core_display_text(TARGET display-text-consumer FILE Empty.elcl)
if(DISPLAY_TEXT_CASE STREQUAL "duplicate")
    erbsland_core_display_text(TARGET display-text-consumer FILE Extra.elcl)
endif()

add_subdirectory(src)
