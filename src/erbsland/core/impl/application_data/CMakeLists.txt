# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        ApplicationCryptologyData.hpp
        ApplicationCryptologyData_fwd.hpp
        ApplicationDataAccessor.hpp
        ApplicationEventData.cpp
        ApplicationEventData.hpp
        ApplicationEventData_fwd.hpp
        ApplicationLifecycleData.cpp
        ApplicationLifecycleData.hpp
        ApplicationLifecycleData_fwd.hpp
        ApplicationLogData.cpp
        ApplicationLogData.hpp
        ApplicationLogData_fwd.hpp
        ApplicationOptionsData.cpp
        ApplicationOptionsData.hpp
        ApplicationOptionsData_fwd.hpp
        ApplicationPartsData.cpp
        ApplicationPartsData.hpp
        ApplicationPartsData_fwd.hpp
        ApplicationRandomData.cpp
        ApplicationRandomData.hpp
        ApplicationRandomData_fwd.hpp
        ApplicationResourceData.hpp
        ApplicationResourceData_fwd.hpp
        ApplicationRuntimeData.hpp
        ApplicationRuntimeData_fwd.hpp
        ApplicationSystemData.cpp
        ApplicationSystemData.hpp
        ApplicationSystemData_fwd.hpp
        ApplicationTerminalData.cpp
        ApplicationTerminalData.hpp
        ApplicationTerminalData_fwd.hpp
)
