# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        DecodedChar.hpp
        Decoder.cpp
        Decoder.hpp
        Decoder_fwd.hpp
        DecoderState.hpp
        FastNameDecoder.cpp
        FastNameDecoder.hpp
        TokenDecoder.cpp
        TokenDecoder.hpp
        TokenDecoder_fwd.hpp
        Transaction.cpp
        Transaction.hpp
        Transaction_fwd.hpp
)
