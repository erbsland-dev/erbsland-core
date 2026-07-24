# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(impl)

target_sources(regex-api-profile PRIVATE
        Application.hpp
        Application_fwd.hpp
        BuildIdentity.hpp.in
        CorpusDirectory.hpp.in
        DefaultConfiguration.hpp.in
        main.cpp
        ProfileTypes.cpp
        ProfileTypes.hpp
)
