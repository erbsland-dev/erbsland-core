# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        U8Encoding.hpp
        U8StringAppendTools.cpp
        U8StringAppendTools.hpp
        U8StringBuilder.cpp
        U8StringBuilder.hpp
        U8StringBuilder_fwd.hpp
        U8StringCharReadTool.cpp
        U8StringCharReadTool.hpp
        U8StringComparisonTools.cpp
        U8StringComparisonTools.hpp
        U8StringData.cpp
        U8StringData.hpp
        U8StringData_fwd.hpp
        U8StringDataView.hpp
        U8StringDataView_fwd.hpp
        U8StringEncodingTools.cpp
        U8StringEncodingTools.hpp
        U8StringEncodingTools_fwd.hpp
        U8StringLiteralFactory.hpp
        U8StringLiteralFactory_fwd.hpp
        U8StringLiteralStorage.hpp
        U8StringModifyTools.cpp
        U8StringModifyTools.hpp
        U8StringModifyTools_replace.cpp
        U8StringReader.cpp
        U8StringReader.hpp
        U8StringReader_fwd.hpp
        U8StringReadTools.cpp
        U8StringReadTools.hpp
        U8StringSharedStorage.cpp
        U8StringSharedStorage.hpp
        U8StringSharedStorage_fwd.hpp
        U8StringStorage.hpp
        U8StringTraits.hpp
        U8StringTransformTools.cpp
        U8StringTransformTools.hpp
        U8StringTrimTools.cpp
        U8StringTrimTools.hpp
        U8Writer.hpp
)
