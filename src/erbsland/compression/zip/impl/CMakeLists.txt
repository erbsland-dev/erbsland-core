# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        ArchiveItem.cpp
        ArchiveItem.hpp
        ArchiveItem_fwd.hpp
        ArchiveReader.cpp
        ArchiveReader.hpp
        ArchiveReader_extract.cpp
        ArchiveReader_records.cpp
        ArchiveReader_stream.cpp
        ArchiveReader_validation.cpp
        ArchiveWriter.cpp
        ArchiveWriter.hpp
        ArchiveWriter_records.cpp
        ArchiveWriter_stream.cpp
        ZipEntryRecord.hpp
        ZipTools.cpp
        ZipTools.hpp
)
