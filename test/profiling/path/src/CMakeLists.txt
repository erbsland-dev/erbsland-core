# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

target_sources(path-walker-profile PRIVATE
        Application.hpp
        Application_fwd.hpp
        DefaultConfiguration.hpp.in
        PathWalkMethod.hpp
        PathWalkerScenarioWorkload.cpp
        PathWalkerScenarioWorkload.hpp
        PathWalkerScenarioWorkload_fwd.hpp
        PathWalkerProfileApplication.cpp
        PathWalkerWorkerWorkload.cpp
        PathWalkerWorkerWorkload.hpp
        PathWalkerWorkerWorkload_fwd.hpp
        main.cpp
)
