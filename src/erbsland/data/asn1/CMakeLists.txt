# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        Asn1BitString.hpp
        Asn1Limits.hpp
        Asn1Node.cpp
        Asn1Node.hpp
        Asn1Node_fwd.hpp
        Asn1ObjectIdentifier.cpp
        Asn1ObjectIdentifier.hpp
        Asn1Tag.hpp
        Asn1TagClass.hpp
        Asn1UniversalType.hpp
)
