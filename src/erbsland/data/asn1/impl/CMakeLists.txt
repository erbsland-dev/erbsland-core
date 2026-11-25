# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Asn1Decoder.cpp
        Asn1Decoder.hpp
        Asn1Decoder_primitive.cpp
        Asn1ObjectIdentifierCodec.cpp
        Asn1ObjectIdentifierCodec.hpp
        Asn1Writer.cpp
        Asn1Writer.hpp
        DecodedNode.hpp
        DecodedNode_fwd.hpp
        PrimitiveContent.cpp
        PrimitiveContent.hpp
)
