# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        BestGrowth.cpp
        BestGrowth.hpp
        ByteBlockData.hpp
        ByteBlockData_fwd.hpp
        ByteBufferData.hpp
        ByteBufferData_fwd.hpp
        ByteComparisonTools.hpp
        ByteDataView.hpp
        ByteDataView_fwd.hpp
        ByteIntegerAccess.hpp
        ByteModifyTools.hpp
        ByteReadTools.hpp
        ByteSequenceOperations.hpp
        RingBufferStorageTraits.hpp
        RingBufferWriter.hpp
        SecureErase.cpp
        SecureErase.hpp
        SecureEraseBackend.hpp
        SharedArrayCapacity.hpp
        SharedArrayDataTraits.hpp
        SharedDataPointerTraits.hpp
        SharedDataPointerTraits_fwd.hpp
        SharedByteDataWithFlag.hpp
        SharedByteDataWithFlag_fwd.hpp
        StorageIdentifierImpl.hpp
        Throw.cpp
        Throw.hpp
        UnsafeByteArrayAccess.hpp
        UnsafeByteArrayAccess_fwd.hpp
        UnsafeByteBlockAccess.cpp
        UnsafeByteBlockAccess.hpp
        UnsafeByteBlockAccess_fwd.hpp
        UnsafeByteBlockBuffer.hpp
        UnsafeByteBlockBuffer_fwd.hpp
        UnsafeByteBufferAccess.hpp
        UnsafeByteBufferAccess_fwd.hpp
        UnsafeRingBufferAccess.hpp
        UnsafeRingBufferAccess_fwd.hpp
)

if (WIN32)
    target_sources(erbsland-core PRIVATE
            SecureEraseWindows.cpp
    )
elseif (APPLE)
    target_sources(erbsland-core PRIVATE
            SecureEraseMacos.cpp
    )
else ()
    target_sources(erbsland-core PRIVATE
            SecureEraseLinux.cpp
    )
endif ()
