# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(char-set-profile PRIVATE
        Application.hpp
        Application_fwd.hpp
        CharSetProfileApplication.cpp
        CharSetFixture.hpp
        CharSetScenarioWorkload.cpp
        CharSetScenarioWorkload.hpp
        CharSetScenarioWorkload_fwd.hpp
        CharSetWorkerWorkload.cpp
        CharSetWorkerWorkload.hpp
        CharSetWorkerWorkload_fwd.hpp
        DefaultConfiguration.hpp.in
        Operation.hpp
        OperationDescriptor.cpp
        OperationDescriptor.hpp
        main.cpp
)
