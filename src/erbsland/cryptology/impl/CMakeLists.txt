# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(algorithm)
add_subdirectory(hash)

target_sources(erbsland-core PRIVATE
        PasswordHashData.hpp
        PasswordHashData_fwd.hpp
        PasswordHashFormat.cpp
        PasswordHashFormat.hpp
        SecureEraseGuard.hpp
)
