# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        BestGrowth.cpp
        BestGrowth.hpp
        BestGrowthStrategy.hpp
        BitReader_fwd.hpp
        BitReaderImpl.cpp
        BitReaderImpl.hpp
        BitReaderImplFor.hpp
        BitWriter_fwd.hpp
        BitWriterImpl.cpp
        BitWriterImpl.hpp
        BitWriterImplFor.hpp
        ByteArrayErrors.cpp
        ByteArrayErrors.hpp
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
        ByteReaderTools.cpp
        ByteReaderTools.hpp
        ByteReaderTools_fwd.hpp
        ByteReadTools.cpp
        ByteReadTools.hpp
        ByteTextTools.cpp
        ByteTextTools.hpp
        ByteWriterTools.cpp
        ByteWriterTools.hpp
        ByteWriteTools.cpp
        ByteWriteTools.hpp
        RingBufferStorageTraits.hpp
        RingBufferWriter.hpp
        SecureErase.cpp
        SecureErase.hpp
        SecureEraseBackend.hpp
        SharedArrayCapacity.hpp
        SharedArrayDataTraits.hpp
        SharedArrayDataTraits.tpp
        SharedByteDataWithFlag.hpp
        SharedByteDataWithFlag_fwd.hpp
        SharedDataPointerTraits.hpp
        SharedDataPointerTraits_fwd.hpp
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
