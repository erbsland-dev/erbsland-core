# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(configuration_signatures)
target_sources(configuration_signatures PRIVATE
        ConfigurationSignaturesDemos.hpp
        HmacSignatureSigner.hpp
        HmacSignatureValidator.hpp
        main.cpp
        SignatureDemoSupport.hpp
        SignDocuments.cpp
        ValidateSignatures.cpp
)
