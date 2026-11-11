# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(file_search PRIVATE
        FileSearchApp.cpp
        FileSearchApp.hpp
        main.cpp
)
target_sources(erbsland-core-file-search-support PRIVATE
        QueryEditor.cpp
        QueryEditor.hpp
        SearchResults.cpp
        SearchResults.hpp
        SearchScanner.cpp
        SearchScanner.hpp
        SearchUpdate.hpp
        SearchView.cpp
        SearchView.hpp
)
