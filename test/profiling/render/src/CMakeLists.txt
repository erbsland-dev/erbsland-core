# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

target_sources(render-profile PRIVATE
        Application.hpp
        Application_fwd.hpp
        Corpus.cpp
        Corpus.hpp
        CorpusDirectory.hpp.in
        DefaultConfiguration.hpp.in
        LayoutGraphCompiler.cpp
        LayoutGraphCompiler.hpp
        main.cpp
        ProfileTypes.hpp
        RenderProfileApplication.cpp
        RenderScenarioWorkload.cpp
        RenderScenarioWorkload.hpp
        RenderScenarioWorkload_fwd.hpp
        RenderWorkerWorkload.cpp
        RenderWorkerWorkload.hpp
        RenderWorkerWorkload_fwd.hpp
        ValidationDigest.hpp
)
