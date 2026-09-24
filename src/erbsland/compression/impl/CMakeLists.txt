# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(bzip2)
add_subdirectory(deflate)
add_subdirectory(lz4)
add_subdirectory(lzma)
add_subdirectory(zstandard)

target_sources(erbsland-core PRIVATE
        CodecBitOutput.hpp
        CodecBitReader.hpp
        CodecOutput.hpp
        CodecReader.hpp
        CompressionCodec.cpp
        CompressionCodec.hpp
        CompressionCodec_fwd.hpp
        CompressionCodec_stream.cpp
        CompressionCodec_transfer.cpp
        CompressionCrc32.hpp
        CompressionErrorDiagnostic.cpp
        CompressionErrorDiagnostic.hpp
        CompressionTransferOperation.cpp
        CompressionTransferOperation.hpp
)
