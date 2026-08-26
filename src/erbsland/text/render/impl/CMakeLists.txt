# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        BuiltInFilter.cpp
        BuiltInFilter.hpp
        BuiltInFilters.cpp
        BuiltInFilters.hpp
        CompiledBlock.hpp
        CompiledBlock_fwd.hpp
        CompiledExtends.hpp
        CompiledExtends_fwd.hpp
        CompiledInclude.hpp
        CompiledInclude_fwd.hpp
        CompiledLayout.cpp
        CompiledLayout.hpp
        CompiledLayout_fwd.hpp
        Compiler.cpp
        Compiler.hpp
        Compiler_dependencies.cpp
        Engine.cpp
        Engine.hpp
        Engine_control.cpp
        Engine_operations.cpp
        Engine_values.cpp
        Environment.cpp
        Environment.hpp
        ExecutionFrame.cpp
        ExecutionFrame.hpp
        ExpressionCompiler.cpp
        ExpressionCompiler.hpp
        ExpressionCompiler_filters.cpp
        FileSystemLoader.cpp
        FileSystemLoader.hpp
        LoopState.cpp
        LoopState.hpp
        LoopState_fwd.hpp
        Opcode.hpp
        Program.cpp
        Program.hpp
        Program_fwd.hpp
        ProgramError.cpp
        ProgramError.hpp
        ProgramInstruction.hpp
        ProgramReader.cpp
        ProgramReader.hpp
        ProgramWriter.cpp
        ProgramWriter.hpp
        RenderCounters.hpp
        ResourceLoader.cpp
        ResourceLoader.hpp
        Token.hpp
        Tokenizer.cpp
        Tokenizer.hpp
        Tokenizer_literals.cpp
        TokenKind.hpp
        ValueComparison.cpp
        ValueComparison.hpp
        ValueData.cpp
        ValueData.hpp
        ValueData_fwd.hpp
        ValueDataTraits.hpp
        ValueTest.hpp
)
