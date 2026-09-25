# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        EnvironmentSource.cpp
        EnvironmentSource.hpp
        EscapeMode.hpp
        Filter.cpp
        Filter.hpp
        Filter_fwd.hpp
        Replacer.cpp
        Replacer.hpp
        Replacer_fwd.hpp
        ReplacerError.hpp
        ReplacerErrorCategory.hpp
        ReplacerOptions.cpp
        ReplacerOptions.hpp
        Source.cpp
        Source.hpp
        Source_fwd.hpp
        TextFilter.cpp
        TextFilter.hpp
        VariableSource.cpp
        VariableSource.hpp
)
