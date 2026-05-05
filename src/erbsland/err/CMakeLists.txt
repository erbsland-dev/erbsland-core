# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        ApplicationError.cpp
        ApplicationError.hpp
        EncodingError.hpp
        Exception.cpp
        Exception.hpp
        FormatError.hpp
        OptionError.cpp
        OptionError.hpp
        OutOfRangeError.hpp
        OverflowError.hpp
        ParseError.cpp
        ParseError.hpp
        ParseNumberError.hpp
        RandomError.hpp
        StreamError.hpp
        ThrowHelper.cpp
        ThrowHelper.hpp
        U16EncodingError.hpp
        U32EncodingError.hpp
        U8EncodingError.hpp
)
