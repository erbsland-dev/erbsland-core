# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(impl)

target_sources(string-types-profile PRIVATE
        Application.hpp
        Application_fwd.hpp
        Configuration.cpp
        Configuration.hpp
        DefaultConfiguration.hpp.in
        main.cpp
        ProfileTypes.cpp
        ProfileTypes.hpp
        Workload.cpp
        Workload.hpp
)
