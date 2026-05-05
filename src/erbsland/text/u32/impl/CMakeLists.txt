# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        U32Encoding.hpp
        U32StringAppendTools.cpp
        U32StringAppendTools.hpp
        U32StringBuilder.cpp
        U32StringBuilder.hpp
        U32StringComparisonTools.cpp
        U32StringComparisonTools.hpp
        U32StringData.cpp
        U32StringData.hpp
        U32StringDataView.hpp
        U32StringDataView_fwd.hpp
        U32StringEncodingTools.cpp
        U32StringEncodingTools.hpp
        U32StringEncodingTools_fwd.hpp
        U32StringLiteralFactory.hpp
        U32StringLiteralFactory_fwd.hpp
        U32StringLiteralStorage.hpp
        U32StringModifyTools.cpp
        U32StringModifyTools.hpp
        U32StringReader.cpp
        U32StringReader.hpp
        U32StringReader_fwd.hpp
        U32StringReadTools.cpp
        U32StringReadTools.hpp
        U32StringSharedStorage.cpp
        U32StringSharedStorage.hpp
        U32StringTraits.hpp
        U32StringTransformTools.cpp
        U32StringTransformTools.hpp
        U32StringTrimTools.cpp
        U32StringTrimTools.hpp
        U32StringViewStorage.hpp
        U32Writer.hpp
)
