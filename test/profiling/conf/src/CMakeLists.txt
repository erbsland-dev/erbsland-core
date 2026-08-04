# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

target_sources(conf-parser-profile PRIVATE
        Application.hpp
        Application_fwd.hpp
        ConfParserProfileApplication.cpp
        DefaultConfiguration.hpp.in
        EmbeddedDocument.hpp
        EmbeddedDocument_fwd.hpp
        EmbeddedDocuments.cpp
        EmbeddedDocuments.hpp
        main.cpp
        ParserScenarioWorkload.cpp
        ParserScenarioWorkload.hpp
        ParserScenarioWorkload_fwd.hpp
        ParserWorkerWorkload.cpp
        ParserWorkerWorkload.hpp
        ParserWorkerWorkload_fwd.hpp
)
