# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        BufferedByteInputStream.cpp
        BufferedByteInputStream.hpp
        BufferedByteInputStreamData.cpp
        BufferedByteInputStreamData.hpp
        BufferedByteInputStreamData_fwd.hpp
        BufferedByteOutputStream.cpp
        BufferedByteOutputStream.hpp
        BufferedByteOutputStreamData.cpp
        BufferedByteOutputStreamData.hpp
        BufferedByteOutputStreamData_fwd.hpp
        EncodedTextInputStream.cpp
        EncodedTextInputStream.hpp
        EncodedTextOutputStream.cpp
        EncodedTextOutputStream.hpp
        InputStreamFactory.cpp
        InputStreamFactory.hpp
        IoService.cpp
        IoService.hpp
        NativeByteStream.hpp
        NativeOutputStream.cpp
        NativeOutputStream.hpp
        PosixNativeStream.hpp
        PrintContextCommonBuilder.cpp
        PrintContextCommonBuilder.hpp
        PrintContextToBuilder.hpp
        PrintContextToWrite.cpp
        PrintContextToWrite.hpp
        RetainedTextBuffer.cpp
        RetainedTextBuffer.hpp
        StandardInputStreamProxy.cpp
        StandardInputStreamProxy.hpp
        StandardStreamProxy.cpp
        StandardStreamProxy.hpp
        StandardStreamRedirectData.cpp
        StandardStreamRedirectData.hpp
        StandardStreamRedirectData_fwd.hpp
        StandardStreamRegistry.cpp
        StandardStreamRegistry.hpp
        StandardStreamSlot.hpp
        StandardTextOutputStream.cpp
        StandardTextOutputStream.hpp
        StreamBufferSizes.cpp
        StreamBufferSizes.hpp
        StreamErrorDiagnostic.cpp
        StreamErrorDiagnostic.hpp
        TextOutputStreamTraits.hpp
        WindowsNativeStream.hpp
)

if(WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsNativeStream.cpp
    )
else()
    target_sources(erbsland-core PRIVATE
            PosixNativeStream.cpp
    )
endif()
