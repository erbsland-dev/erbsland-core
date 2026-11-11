# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(checksum_packer_invoke PRIVATE
        ChecksumPackerApp.hpp
        Constants.hpp
        FileWithDigest.hpp
        main.cpp
        Packer.hpp
        Preparation.hpp
        Scanner.hpp
        SourceAndZipFile.hpp
        State.hpp
        Workflow.hpp
        Workflow_fwd.hpp
)
