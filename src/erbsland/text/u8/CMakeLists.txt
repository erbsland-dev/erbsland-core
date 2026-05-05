# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        U8Format.cpp
        U8Format.hpp
        U8Format.tpp
        U8String.cpp
        U8String.hpp
        U8String_comparison.cpp
        U8String_float.cpp
        U8String_fwd.hpp
        U8String_integer.tpp
        U8String_trim.cpp
        U8StringCharView.cpp
        U8StringCharView.hpp
        U8StringCharView_comparison.cpp
        U8StringCharView_fwd.hpp
        U8StringCharView_trim.cpp
        U8StringCIHashMap.hpp
        U8StringCIHashMap_fwd.hpp
        U8StringCIHashSet.hpp
        U8StringCIHashSet_fwd.hpp
        U8StringCIMap.hpp
        U8StringCIMap_fwd.hpp
        U8StringCISet.hpp
        U8StringCISet_fwd.hpp
        U8StringConstIterator.cpp
        U8StringConstIterator.hpp
        U8StringConstIterator_fwd.hpp
        U8StringHashMap.hpp
        U8StringHashMap_fwd.hpp
        U8StringHashSet.hpp
        U8StringHashSet_fwd.hpp
        U8StringList.hpp
        U8StringList_fwd.hpp
        U8StringLiteral.hpp
        U8StringLiteral.tpp
        U8StringLiteral_fwd.hpp
        U8StringMap.hpp
        U8StringMap_fwd.hpp
        U8StringSet.hpp
        U8StringSet_fwd.hpp
        U8StringView.cpp
        U8StringView.hpp
        U8StringView_comparison.cpp
        U8StringView_fwd.hpp
        U8StringView_integer.tpp
        U8StringView_trim.cpp
        U8StringViewList.hpp
        U8StringViewList_fwd.hpp
)
