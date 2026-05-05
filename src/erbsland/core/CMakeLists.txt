# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        Application.cpp
        Application.hpp
        Application_fwd.hpp
        ApplicationInfo.hpp
        CommandLineArguments.hpp
        Definitions.hpp
        MainFn.hpp
        MakeOneNamespace.hpp
        Namespaces.hpp
)
