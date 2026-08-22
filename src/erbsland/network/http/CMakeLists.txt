# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        HttpCookieSameSite.hpp
        HttpField.cpp
        HttpField.hpp
        HttpFieldName.cpp
        HttpFieldName.hpp
        HttpFieldType.cpp
        HttpFieldType.hpp
        HttpHeaderLimits.hpp
        HttpHeaders.cpp
        HttpHeaders.hpp
        HttpMediaType.cpp
        HttpMediaType.hpp
        HttpMediaTypeParameter.cpp
        HttpMediaTypeParameter.hpp
        HttpMethod.cpp
        HttpMethod.hpp
        HttpMethodType.hpp
        HttpRequestHead.cpp
        HttpRequestHead.hpp
        HttpResponseHead.cpp
        HttpResponseHead.hpp
        HttpStatus.cpp
        HttpStatus.hpp
        HttpVersion.cpp
        HttpVersion.hpp
)
