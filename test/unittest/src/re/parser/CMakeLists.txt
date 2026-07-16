# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        ParserAlternativeLimitsTest.cpp
        ParserAlternativesTest.cpp
        ParserAnchorsTest.cpp
        ParserBase.hpp
        ParserCharacterClassTest.cpp
        ParserCharacterEscapeTest.cpp
        ParserCharacterSequenceTest.cpp
        ParserConstructorTest.cpp
        ParserEmptyAlternativesTest.cpp
        ParserFeaturesTest.cpp
        ParserFlagsAndVerboseTest.cpp
        ParserGroupHandlerEdgeTest.cpp
        ParserGroupQuantifierTest.cpp
        ParserGroupTest.cpp
        ParserNodeIdTest.cpp
        ParserPatternLengthTest.cpp
        ParserPosixCharacterClassTest.cpp
        ParserQuantifierCounterIndexTest.cpp
        ParserQuantifierTest.cpp
        ParserVerboseBehaviorTest.cpp
        PatternNodeTest.cpp
)
