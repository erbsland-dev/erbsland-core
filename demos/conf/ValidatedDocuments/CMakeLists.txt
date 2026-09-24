# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(validated_documents)
target_sources(validated_documents PRIVATE
        DefaultsAndValidation.cpp
        main.cpp
        RuleMetadata.cpp
        SecretValues.cpp
        ValidatedDocumentsDemos.hpp
)
