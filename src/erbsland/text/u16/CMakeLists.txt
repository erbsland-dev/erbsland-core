# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        U16Format.cpp
        U16Format.hpp
        U16Format.tpp
        U16String.cpp
        U16String.hpp
        U16String_comparison.cpp
        U16String_float.cpp
        U16String_fwd.hpp
        U16String_integer.tpp
        U16String_modify.cpp
        U16String_trim.cpp
        U16StringCharView.cpp
        U16StringCharView.hpp
        U16StringCharView_comparison.cpp
        U16StringCharView_fwd.hpp
        U16StringCharView_trim.cpp
        U16StringCIHashMap.hpp
        U16StringCIHashMap_fwd.hpp
        U16StringCIHashSet.hpp
        U16StringCIHashSet_fwd.hpp
        U16StringCIMap.hpp
        U16StringCIMap_fwd.hpp
        U16StringCISet.hpp
        U16StringCISet_fwd.hpp
        U16StringConstIterator.cpp
        U16StringConstIterator.hpp
        U16StringConstIterator_fwd.hpp
        U16StringHashMap.hpp
        U16StringHashMap_fwd.hpp
        U16StringHashSet.hpp
        U16StringHashSet_fwd.hpp
        U16StringList.hpp
        U16StringList_fwd.hpp
        U16StringLiteral.hpp
        U16StringLiteral_fwd.hpp
        U16StringMap.hpp
        U16StringMap_fwd.hpp
        U16StringSet.hpp
        U16StringSet_fwd.hpp
        U16StringView.cpp
        U16StringView.hpp
        U16StringView_comparison.cpp
        U16StringView_fwd.hpp
        U16StringView_integer.tpp
        U16StringView_trim.cpp
        U16StringViewList.hpp
        U16StringViewList_fwd.hpp
)
