# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        CoAsyncGenerator.hpp
        CoAsyncGenerator_fwd.hpp
        CoGenerator.hpp
        CoGenerator_fwd.hpp
        CoTask.hpp
        CoTask_fwd.hpp
        EnumFlags.hpp
        HashHelper.hpp
        HashMap.hpp
        HashMap_algorithm.tpp
        HashMap_change.tpp
        HashMap_construction.tpp
        HashMap_elements.tpp
        HashMap_memory.tpp
        HashMap_remove.tpp
        HashMap_set.tpp
        HashMap_take.tpp
        HashMap_tests.tpp
        HashSet.hpp
        HashSet_algorithm.tpp
        HashSet_change.tpp
        HashSet_construction.tpp
        HashSet_elements.tpp
        HashSet_fwd.hpp
        HashSet_memory.tpp
        HashSet_remove.tpp
        HashSet_sets.tpp
        HashSet_tests.tpp
        List.hpp
        List_algorithm.tpp
        List_change.tpp
        List_construction.tpp
        List_elements.tpp
        List_fwd.hpp
        List_memory.tpp
        List_remove.tpp
        List_search.tpp
        List_slice.tpp
        List_sort.tpp
        List_take.tpp
        List_tests.tpp
        LoopResult.hpp
        LoopStatus.hpp
        Map.hpp
        Map_algorithm.tpp
        Map_change.tpp
        Map_construction.tpp
        Map_elements.tpp
        Map_memory.tpp
        Map_remove.tpp
        Map_set.tpp
        Map_take.tpp
        Map_tests.tpp
        Result.hpp
        ResultWithData.hpp
        Set.hpp
        Set_algorithm.tpp
        Set_change.tpp
        Set_construction.tpp
        Set_elements.tpp
        Set_fwd.hpp
        Set_memory.tpp
        Set_remove.tpp
        Set_sets.tpp
        Set_tests.tpp
)
