# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)
add_subdirectory(zip)

target_sources(erbsland-core PRIVATE
        ByteCompressor.cpp
        ByteCompressor.hpp
        ByteCompressor_fwd.hpp
        ByteDecompressor.cpp
        ByteDecompressor.hpp
        ByteDecompressor_fwd.hpp
        CompressionAlgorithm.cpp
        CompressionAlgorithm.hpp
        CompressionError.cpp
        CompressionError.hpp
        CompressionErrorContext.hpp
        CompressionErrorReason.hpp
        CompressionFallbackPolicy.hpp
        CompressionFormat.hpp
        CompressionLevel.hpp
        CompressionOptions.hpp
        CompressionPhase.hpp
        CompressionProgress.hpp
        CompressionProgressAction.hpp
        CompressionTransferOptions.hpp
        CompressionTransferResult.hpp
        DecompressionOptions.hpp
)
