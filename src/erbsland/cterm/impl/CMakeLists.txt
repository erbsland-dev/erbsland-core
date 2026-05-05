# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(paragraph)

target_sources(erbsland-core PRIVATE
        BitmapPainter.cpp
        BitmapPainter.hpp
        BlockPrintContextToString.cpp
        BlockPrintContextToString.hpp
        BlockPrintContextToTerminal.cpp
        BlockPrintContextToTerminal.hpp
        BlockStringBuilder.hpp
        BlockStringData.cpp
        BlockStringData.hpp
        BlockStringRangeView.cpp
        BlockStringRangeView.hpp
        BlockStringWrapper.cpp
        BlockStringWrapper.hpp
        BlockTextPainter.cpp
        BlockTextPainter.hpp
        BlockTextUtil.hpp
        CombinedBlock.cpp
        CombinedBlock.hpp
        CommonBoxFrameBlockCombinationStyle.cpp
        CommonBoxFrameBlockCombinationStyle.hpp
        CommonBoxFrameBlockCombinationStyle_data.cpp
        FontDefaultAscii.cpp
        FrameBorder_data.hpp
        FramePainter.cpp
        FramePainter.hpp
        HashHelper.hpp
        InputBackend.cpp
        InputBackend.hpp
        KeyDecoder.cpp
        KeyDecoder.hpp
        LineBuffer.hpp
        SaturatingMath.hpp
        TypeTraits.hpp
)

if (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsBackend.cpp
            WindowsBackend.hpp
            WindowsSignalDispatcher.cpp
            WindowsSignalDispatcher.hpp
    )
else ()
    target_sources(erbsland-core PRIVATE
            PosixBackend.cpp
            PosixBackend.hpp
            PosixSignalDispatcher.cpp
            PosixSignalDispatcher.hpp
    )
endif ()
