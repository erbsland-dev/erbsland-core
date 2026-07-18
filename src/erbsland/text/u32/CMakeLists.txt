# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        U32Format.cpp
        U32Format.hpp
        U32Format.tpp
        U32String.cpp
        U32String.hpp
        U32String_comparison.cpp
        U32String_fwd.hpp
        U32String_integer.tpp
        U32String_trim.cpp
        U32StringCIHashMap.hpp
        U32StringCIHashMap_fwd.hpp
        U32StringCIHashSet.hpp
        U32StringCIHashSet_fwd.hpp
        U32StringCIMap.hpp
        U32StringCIMap_fwd.hpp
        U32StringCISet.hpp
        U32StringCISet_fwd.hpp
        U32StringConstIterator.cpp
        U32StringConstIterator.hpp
        U32StringConstIterator_fwd.hpp
        U32StringEditor.cpp
        U32StringEditor.hpp
        U32StringEditor_comparison.cpp
        U32StringEditor_float.cpp
        U32StringEditor_fwd.hpp
        U32StringEditor_integer.tpp
        U32StringEditor_trim.cpp
        U32StringEditorList.hpp
        U32StringEditorList_fwd.hpp
        U32StringHashMap.hpp
        U32StringHashMap_fwd.hpp
        U32StringHashSet.hpp
        U32StringHashSet_fwd.hpp
        U32StringList.hpp
        U32StringList_fwd.hpp
        U32StringLiteral.hpp
        U32StringLiteral_fwd.hpp
        U32StringMap.hpp
        U32StringMap_fwd.hpp
        U32StringSet.hpp
        U32StringSet_fwd.hpp
)
