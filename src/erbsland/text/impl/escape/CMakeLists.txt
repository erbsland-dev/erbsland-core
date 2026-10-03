# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        ConfigEscapeFormatter.cpp
        ConfigEscapeFormatter.hpp
        CppEscapeFormatter.cpp
        CppEscapeFormatter.hpp
        DisplayEscapeFormatter.cpp
        DisplayEscapeFormatter.hpp
        EscapeFormatter.cpp
        EscapeFormatter.hpp
        EscapeFormatter_fwd.hpp
        HtmlEscapeFormatter.cpp
        HtmlEscapeFormatter.hpp
        JsonEscapeFormatter.cpp
        JsonEscapeFormatter.hpp
        LogEscapeFormatter.cpp
        LogEscapeFormatter.hpp
        MarkdownEscapeFormatter.cpp
        MarkdownEscapeFormatter.hpp
        NoneEscapeFormatter.cpp
        NoneEscapeFormatter.hpp
        RegExEscapeFormatter.cpp
        RegExEscapeFormatter.hpp
        StringSafeTransformTools.hpp
        UrlEscapeFormatter.cpp
        UrlEscapeFormatter.hpp
        XmlEscapeFormatter.cpp
        XmlEscapeFormatter.hpp
)
