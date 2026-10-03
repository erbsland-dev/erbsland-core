# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        Diagnostic.cpp
        Diagnostic.hpp
        DiagnosticHelper.cpp
        DiagnosticHelper.hpp
        ErrorDocumentBuilder.cpp
        ErrorDocumentBuilder.hpp
        Exception.cpp
        Exception.hpp
        Exception_fwd.hpp
        LogicFailure.hpp
        NotFoundError.hpp
        OutOfRangeError.hpp
        OutOfRangeFailure.hpp
        OverflowError.hpp
        ParameterFailure.cpp
        ParameterFailure.hpp
        ParseError.cpp
        ParseError.hpp
        RuntimeError.hpp
)
