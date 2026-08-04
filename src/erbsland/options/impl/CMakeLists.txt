# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        AsciiOptionText.hpp
        ExecutableName.cpp
        ExecutableName.hpp
        OptionDisplayGroup.hpp
        OptionDisplayGroup_fwd.hpp
        OptionDisplayModel.cpp
        OptionDisplayModel.hpp
        OptionDisplayModel_fwd.hpp
        OptionDisplayRow.hpp
        OptionDisplayRow_fwd.hpp
        OptionDocumentBuilder.cpp
        OptionDocumentBuilder.hpp
        OptionDocumentBuilder_error.cpp
        OptionErrorDiagnostic.cpp
        OptionErrorDiagnostic.hpp
        OptionParsedValue.hpp
        OptionParsedValue_fwd.hpp
        OptionParser.cpp
        OptionParser.hpp
        OptionParser_callbacks.cpp
        OptionParser_fwd.hpp
        OptionParser_lookup.cpp
        OptionParser_parse.cpp
        OptionParser_positionals.cpp
        OptionParserStorage.cpp
        OptionParserStorage.hpp
)
