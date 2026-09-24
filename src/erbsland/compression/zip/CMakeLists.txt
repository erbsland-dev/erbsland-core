# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        ArchiveDirectoryOptions.hpp
        ArchiveEntryOptions.hpp
        ArchiveExtractionOptions.hpp
        ArchiveItem.hpp
        ArchiveItem_fwd.hpp
        ArchiveReader.cpp
        ArchiveReader.hpp
        ArchiveReader_fwd.hpp
        ArchiveReaderOptions.hpp
        ArchiveWriter.cpp
        ArchiveWriter.hpp
        ArchiveWriter_fwd.hpp
        ArchiveWriterOptions.hpp
        CompressionMethod.hpp
        Zip64Policy.hpp
        ZipError.cpp
        ZipError.hpp
        ZipErrorContext.hpp
        ZipErrorReason.hpp
        ZipOperationPhase.hpp
        ZipProgress.hpp
)
