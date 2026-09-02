# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Chars.cpp
        Chars.hpp
        ConfKey.cpp
        ConfKey.hpp
        ConstraintAttribute.cpp
        ConstraintAttribute.hpp
        ConstraintOptions.cpp
        ConstraintOptions.hpp
        Contains.hpp
        Ends.hpp
        Equals.cpp
        Equals.hpp
        In.cpp
        In.hpp
        Matches.cpp
        Matches.hpp
        Maximum.cpp
        Maximum.hpp
        Minimum.cpp
        Minimum.hpp
        Multiple.cpp
        Multiple.hpp
        Starts.hpp
        StringPartConstraint.cpp
        StringPartConstraint.hpp
)
