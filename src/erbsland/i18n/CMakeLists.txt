# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        all.hpp
        DisplayText.cpp
        DisplayText.hpp
        DisplayText_fwd.hpp
        DisplayTextData.cpp
        DisplayTextData.hpp
        DisplayTextData_fwd.hpp
        DisplayTextTranslator.hpp
)
