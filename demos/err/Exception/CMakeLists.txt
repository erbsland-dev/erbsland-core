# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(exception)
target_sources(exception PRIVATE
        ApplicationReporting.cpp
        BuiltInExceptions.cpp
        CauseChains.cpp
        ChoosingFailureMechanism.cpp
        ContextPattern.cpp
        DomainExceptions.cpp
        ExceptionDemos.hpp
        ExceptionText.cpp
        main.cpp
        MinimalCustomException.cpp
        ThrowAndCatch.cpp
)
