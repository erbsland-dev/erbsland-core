# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

target_sources(cryptology-hash-profile PRIVATE
        Application.hpp
        Application_fwd.hpp
        CryptologyHashProfileApplication.cpp
        DefaultConfiguration.hpp.in
        HashScenarioWorkload.cpp
        HashScenarioWorkload.hpp
        HashScenarioWorkload_fwd.hpp
        HashWorkerWorkload.cpp
        HashWorkerWorkload.hpp
        HashWorkerWorkload_fwd.hpp
        main.cpp
)
