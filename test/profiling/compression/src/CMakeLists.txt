# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

target_sources(compression-profile PRIVATE
        Application.hpp
        Application_fwd.hpp
        CompressionProfileApplication.cpp
        CompressionScenarioWorkload.cpp
        CompressionScenarioWorkload.hpp
        CompressionScenarioWorkload_fwd.hpp
        CompressionWorkerWorkload.cpp
        CompressionWorkerWorkload.hpp
        CompressionWorkerWorkload_fwd.hpp
        Corpus.cpp
        Corpus.hpp
        CorpusDirectory.hpp.in
        DefaultConfiguration.hpp.in
        ProfileTypes.hpp
        main.cpp
)
