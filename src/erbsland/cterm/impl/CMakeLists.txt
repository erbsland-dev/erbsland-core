# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(document_renderer)
add_subdirectory(paragraph)

target_sources(erbsland-core PRIVATE
        AnsiSequence.cpp
        AnsiSequence.hpp
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
        LineBuffer.cpp
        LineBuffer.hpp
        ReadLine.cpp
        ReadLine.hpp
        ReadLineBase.cpp
        ReadLineBase.hpp
        ReadLineBase_render.cpp
        ReadSecret.cpp
        ReadSecret.hpp
        StandardInput.cpp
        StandardInput.hpp
        TerminalDocumentStyleData.cpp
        TerminalDocumentStyleData.hpp
        TypeTraits.hpp
)

if (WIN32)
    target_sources(erbsland-core PRIVATE
            InputRecordEraseGuard.hpp
            WindowsBackend.cpp
            WindowsBackend.hpp
            WindowsBackend_input.cpp
            WindowsBackendPrivate.hpp
            WindowsBackendPrivate_fwd.hpp
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
