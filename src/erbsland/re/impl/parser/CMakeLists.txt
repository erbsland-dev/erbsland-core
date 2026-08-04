# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        AnchorAndSpecialHandler.hpp
        CharacterClassHandler.hpp
        CharacterClassState.hpp
        CharClassHandlerState.cpp
        CharClassHandlerState_escape.cpp
        CharClassHandlerState_posix.cpp
        EscapeSequenceHandler.hpp
        EscapeSequences.hpp
        GroupHandler.hpp
        Parser.cpp
        Parser.hpp
        ParserState.hpp
        PatternNode.cpp
        PatternNode.hpp
        PatternNode_fwd.hpp
        PatternNodeAllocator.hpp
        PatternNodeArena.hpp
        QuantifierHandler.hpp
        RegularCharHandler.hpp
)
