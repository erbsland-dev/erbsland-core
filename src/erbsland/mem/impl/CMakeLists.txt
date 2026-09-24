# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        BitReaderImpl.cpp
        BitReaderImpl.hpp
        BitReaderImplFor.hpp
        BitReader_fwd.hpp
        BitWriterImpl.cpp
        BitWriterImpl.hpp
        BitWriterImplFor.hpp
        BitWriter_fwd.hpp
        ByteArrayErrors.cpp
        ByteArrayErrors.hpp
        BestGrowth.cpp
        BestGrowth.hpp
        BestGrowthStrategy.hpp
        ByteBlockData.hpp
        ByteBlockData_fwd.hpp
        ByteBlockLiteralFactory.cpp
        ByteBlockLiteralFactory_fwd.hpp
        ByteBlockStorage.hpp
        ByteBufferData.hpp
        ByteBufferData_fwd.hpp
        ByteComparisonTools.hpp
        ByteDataView.hpp
        ByteDataView_fwd.hpp
        ByteIntegerAccess.hpp
        ByteIntegerTypes.hpp
        ByteModifyTools.hpp
        ByteReadTools.cpp
        ByteReadTools.hpp
        ByteReaderTools.cpp
        ByteReaderTools.hpp
        ByteReaderTools_fwd.hpp
        ByteTextTools.cpp
        ByteTextTools.hpp
        ByteWriteTools.cpp
        ByteWriteTools.hpp
        ByteWriterTools.cpp
        ByteWriterTools.hpp
        RingBufferStorageTraits.hpp
        RingBufferWriter.hpp
        SecureErase.cpp
        SecureErase.hpp
        SecureEraseBackend.hpp
        SharedArrayCapacity.hpp
        SharedArrayDataTraits.hpp
        SharedArrayDataTraits.tpp
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
