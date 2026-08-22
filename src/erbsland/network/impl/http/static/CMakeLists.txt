# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        HttpStaticContent.hpp
        HttpStaticContentOperation.cpp
        HttpStaticContentOperation.hpp
        HttpStaticContentUse.cpp
        HttpStaticContentUse.hpp
        HttpStaticContentUse_fwd.hpp
        HttpStaticFileContent.cpp
        HttpStaticFileContent.hpp
        HttpStaticFileHandler.cpp
        HttpStaticFileHandler.hpp
        HttpStaticResourceContent.cpp
        HttpStaticResourceContent.hpp
        HttpStaticResourceHandler.cpp
        HttpStaticResourceHandler.hpp
)
