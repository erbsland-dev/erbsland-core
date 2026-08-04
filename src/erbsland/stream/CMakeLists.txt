# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        AnyStringBuilderStream.cpp
        AnyStringBuilderStream.hpp
        AnyStringBuilderStream_fwd.hpp
        ByteInputStream.cpp
        ByteInputStream.hpp
        ByteInputStream_fwd.hpp
        ByteOutputStream.cpp
        ByteOutputStream.hpp
        ByteOutputStream_fwd.hpp
        InputStream.hpp
        InputStream_fwd.hpp
        InputStreamSettings.hpp
        OutputStream.hpp
        OutputStream_fwd.hpp
        OutputStreamSettings.cpp
        OutputStreamSettings.hpp
        SensitiveInput.hpp
        SensitiveInputScope.cpp
        SensitiveInputScope.hpp
        SensitiveInputToken.cpp
        StandardStreamRedirect.cpp
        StandardStreamRedirect.hpp
        StandardStreams.cpp
        StandardStreams.hpp
        StreamBuffering.hpp
        StreamCloseStatus.hpp
        StreamError.cpp
        StreamError.hpp
        StreamErrorContext.cpp
        StreamErrorContext.hpp
        StreamErrorSource.cpp
        StreamErrorSource.hpp
        StreamPositioning.cpp
        StreamPositioning.hpp
        StreamPositionOrigin.hpp
        StreamPositionStatus.hpp
        StreamReadResult.hpp
        StreamReadStatus.hpp
        StreamState.hpp
        StreamWaitStatus.hpp
        StreamWriteStatus.hpp
        TempByteOutputStream.cpp
        TempByteOutputStream.hpp
        TempByteOutputStream_fwd.hpp
        TempTextOutputStream.cpp
        TempTextOutputStream.hpp
        TempTextOutputStream_fwd.hpp
        TextInputStream.cpp
        TextInputStream.hpp
        TextInputStream_fwd.hpp
        TextOutputStream.cpp
        TextOutputStream.hpp
        TextOutputStream_fwd.hpp
        TextPrintContext.hpp
        TextPrintContext_fwd.hpp
)
