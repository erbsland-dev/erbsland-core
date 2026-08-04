# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        U16Encoding.hpp
        U16StringAppendTools.cpp
        U16StringAppendTools.hpp
        U16StringBuilder.cpp
        U16StringBuilder.hpp
        U16StringBuilder_fwd.hpp
        U16StringCharReadTool.cpp
        U16StringCharReadTool.hpp
        U16StringComparisonTools.cpp
        U16StringComparisonTools.hpp
        U16StringData.cpp
        U16StringData.hpp
        U16StringData_fwd.hpp
        U16StringDataView.hpp
        U16StringDataView_fwd.hpp
        U16StringEncodingTools.cpp
        U16StringEncodingTools.hpp
        U16StringEncodingTools_fwd.hpp
        U16StringLiteralFactory.hpp
        U16StringLiteralFactory_fwd.hpp
        U16StringLiteralStorage.hpp
        U16StringModifyTools.cpp
        U16StringModifyTools.hpp
        U16StringModifyTools_replace.cpp
        U16StringReader.cpp
        U16StringReader.hpp
        U16StringReader_fwd.hpp
        U16StringReadTools.cpp
        U16StringReadTools.hpp
        U16StringSharedStorage.cpp
        U16StringSharedStorage.hpp
        U16StringSharedStorage_fwd.hpp
        U16StringStorage.hpp
        U16StringTraits.hpp
        U16StringTransformTools.cpp
        U16StringTransformTools.hpp
        U16StringTrimTools.cpp
        U16StringTrimTools.hpp
        U16Writer.hpp
)
