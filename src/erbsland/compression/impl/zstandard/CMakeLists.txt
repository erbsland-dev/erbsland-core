# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        ZstandardChecksum.cpp
        ZstandardChecksum.hpp
        ZstandardCodec.cpp
        ZstandardCodec.hpp
        ZstandardDecoder.cpp
        ZstandardDecoder.hpp
        ZstandardEncoder.cpp
        ZstandardEncoder.hpp
        ZstandardForwardBitReader.cpp
        ZstandardForwardBitReader.hpp
        ZstandardFrame.cpp
        ZstandardFrame.hpp
        ZstandardFseEntry.hpp
        ZstandardFseTable.cpp
        ZstandardFseTable.hpp
        ZstandardHuffmanCode.hpp
        ZstandardHuffmanEncoder.cpp
        ZstandardHuffmanEncoder.hpp
        ZstandardHuffmanTable.cpp
        ZstandardHuffmanTable.hpp
        ZstandardMatch.hpp
        ZstandardReverseBitReader.cpp
        ZstandardReverseBitReader.hpp
        ZstandardReverseBitWriter.cpp
        ZstandardReverseBitWriter.hpp
        ZstandardSequence.hpp
        ZstandardSequenceCode.hpp
        ZstandardSequenceEntry.hpp
        ZstandardSequenceTable.cpp
        ZstandardSequenceTable.hpp
)
