# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Http1BodyFraming.hpp
        Http1CodecLimits.hpp
        Http1DecodeEvent.cpp
        Http1DecodeEvent.hpp
        Http1DecodeEvent_fwd.hpp
        Http1Decoder.cpp
        Http1Decoder.hpp
        Http1Decoder_fields.cpp
        Http1Encoder.cpp
        Http1Encoder.hpp
        Http1FailureReason.hpp
        Http1FramingParser.cpp
        Http1FramingParser.hpp
        Http1ProtocolError.cpp
        Http1ProtocolError.hpp
        Http1RequestDecoder.hpp
        Http1RequestEncoder.hpp
        Http1ResponseDecoder.hpp
        Http1ResponseEncoder.hpp
        Http1Transaction.cpp
        Http1Transaction.hpp
        Http1Transaction_fwd.hpp
        Http1Transaction_io.cpp
        Http1Transaction_lifecycle.cpp
        Http1TransactionCallbacks.hpp
        Http1TransactionFailure.hpp
        Http1TransactionFailure_fwd.hpp
        Http1TransactionOptions.hpp
        Http1TransferCoding.hpp
)
