# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-tool-lib PRIVATE
        Decomposition.hpp
        IdnaMapping.cpp
        IdnaMapping.hpp
        LightDelta.hpp
        LightRange.hpp
        NfcNormalizer.cpp
        NfcNormalizer.hpp
        UcdDataset.cpp
        UcdDataset.hpp
        UcdReader.cpp
        UcdReader.hpp
        UcdRecord.hpp
        UnicodeData.cpp
        UnicodeData.hpp
        UnicodeDataTool.cpp
        UnicodeDataTool.hpp
        UnicodeLightTables.cpp
        UnicodeLightTables.hpp
        UnicodeNormalizationTables.cpp
        UnicodeNormalizationTables.hpp
        UnicodeNormalizationTables_encode.cpp
        UnicodeNormalizationTables_render.cpp
        UnicodeSource.cpp
        UnicodeSource.hpp
)
