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
        U8String_fwd.hpp
        U8String_integer.tpp
        U8String_trim.cpp
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
        U8StringEditor.cpp
        U8StringEditor.hpp
        U8StringEditor_comparison.cpp
        U8StringEditor_float.cpp
        U8StringEditor_fwd.hpp
        U8StringEditor_integer.tpp
        U8StringEditor_modify.cpp
        U8StringEditor_trim.cpp
        U8StringEditorList.hpp
        U8StringEditorList_fwd.hpp
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
        U8StringSplitter.hpp
        U8StringSplitter_fwd.hpp
)
