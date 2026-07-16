# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Argument.cpp
        Argument.hpp
        Assembler.cpp
        Assembler.hpp
        Assembler_Arguments.cpp
        Assembler_Commands.cpp
        Assembler_Operations.cpp
        AssemblerToken.hpp
        AssemblerTokenizer.hpp
        DataSection.hpp
        Disassembler.cpp
        Disassembler.hpp
        DisassemblerFlags.hpp
        GenericProgramReader.hpp
        LabelTarget.hpp
        OperationData.cpp
        OperationData.hpp
        OperationModifier.hpp
)
