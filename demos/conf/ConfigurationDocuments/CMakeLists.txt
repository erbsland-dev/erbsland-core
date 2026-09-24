# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(configuration_documents)
target_sources(configuration_documents PRIVATE
        CompiledRules.cpp
        ConfigurationDocumentsDemos.hpp
        DocumentRules.cpp
        LoadingStrategies.cpp
        main.cpp
        ManualValidation.cpp
        ParseAndAccess.cpp
)
