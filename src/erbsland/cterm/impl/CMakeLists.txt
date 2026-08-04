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
        BlockPrintContextToBlockString.cpp
        BlockPrintContextToBlockString.hpp
        BlockPrintContextToBlockString_fwd.hpp
        BlockPrintContextToCursorWriter.cpp
        BlockPrintContextToCursorWriter.hpp
        BlockPrintContextToString.cpp
        BlockPrintContextToString.hpp
        BlockPrintContextToTerminal.cpp
        BlockPrintContextToTerminal.hpp
        BlockStringBuilder.hpp
        BlockStringBuilder_fwd.hpp
        BlockStringData.cpp
        BlockStringData.hpp
        BlockStringData_fwd.hpp
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
        FrameBorder_data.hpp
        FramePainter.cpp
        FramePainter.hpp
        HashHelper.hpp
        InputBackend.cpp
        InputBackend.hpp
        KeyCodePointParseResult.hpp
        KeyDecoder.cpp
        KeyDecoder.hpp
        KeyParseResult.hpp
        KeyParseStatus.hpp
        LineBuffer.cpp
        LineBuffer.hpp
        LineBuffer_fwd.hpp
        LineBufferEmitLockGuard.cpp
        LineBufferEmitLockGuard.hpp
        ReadLine.cpp
        ReadLine.hpp
        ReadLineBase.cpp
        ReadLineBase.hpp
        ReadLineBase_render.cpp
        ReadLineLayout_fwd.hpp
        ReadLineLayoutTypes.hpp
        ReadLineTestAccess_fwd.hpp
        ReadSecret.cpp
        ReadSecret.hpp
        StandardInput.cpp
        StandardInput.hpp
        TerminalDocumentStyleData.cpp
        TerminalDocumentStyleData.hpp
        TerminalStreamData.cpp
        TerminalStreamData.hpp
        TerminalStreamData_fwd.hpp
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
            WindowsSignalDispatcher_fwd.hpp
    )
else ()
    target_sources(erbsland-core PRIVATE
            PosixBackend.cpp
            PosixBackend.hpp
            PosixSignalDispatcher.cpp
            PosixSignalDispatcher.hpp
            PosixSignalDispatcher_fwd.hpp
    )
endif ()
