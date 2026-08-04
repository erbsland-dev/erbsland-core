# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Asn1Node.cpp
        Asn1Node.hpp
        Asn1Node_fwd.hpp
        Asn1ObjectIdentifier.cpp
        Asn1ObjectIdentifier.hpp
        Asn1TagClass.hpp
        Asn1UniversalType.hpp
)
