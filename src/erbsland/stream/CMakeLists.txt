# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        ByteInputStream.cpp
        ByteInputStream.hpp
        ByteOutputStream.cpp
        ByteOutputStream.hpp
        InputStream.hpp
        OutputStream.hpp
        StandardStreamRedirect.cpp
        StandardStreamRedirect.hpp
        StandardStreams.cpp
        StandardStreams.hpp
        StringBuilderStream.cpp
        StringBuilderStream.hpp
        TextInputStream.cpp
        TextInputStream.hpp
        TextOutputStream.cpp
        TextOutputStream.hpp
        TextOutputStream_fwd.hpp
        TextPrintContext.hpp
)
