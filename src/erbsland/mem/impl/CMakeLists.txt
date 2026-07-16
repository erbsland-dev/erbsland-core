# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        BestGrowth.cpp
        BestGrowth.hpp
        ByteBlockData.hpp
        ByteBlockData_fwd.hpp
        RingBufferStorageTraits.hpp
        SharedArrayCapacity.hpp
        SharedArrayDataTraits.hpp
        SharedDataPointerTraits.hpp
        SharedDataPointerTraits_fwd.hpp
        StorageIdentifierImpl.hpp
        Throw.cpp
        Throw.hpp
        UnsafeByteBlockBuffer.hpp
        UnsafeByteBlockBuffer_fwd.hpp
        UnsafeRingBufferAccess.hpp
        UnsafeRingBufferAccess_fwd.hpp
)
