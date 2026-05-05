# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(string_view)
target_sources(string_view PRIVATE
        BasicTests.cpp
        ByteRangeSlicing.cpp
        CanonicalStationName.cpp
        CaseTransformation.cpp
        CharacterAccess.cpp
        CharacterReplacement.cpp
        CharacterSetCategories.cpp
        CharacterSetCharacters.cpp
        CharacterSetEmpty.cpp
        CharacterSetPatterns.cpp
        CharacterSetReusable.cpp
        CharacterSetText.cpp
        CombineCharacterSets.cpp
        Comparison.cpp
        ComparisonFunctions.cpp
        ConstructionAndStorage.cpp
        CodePointRangeSlicing.cpp
        EfficientLiteralUsage.cpp
        FirstAndLastCharacter.cpp
        FindingAllOccurrences.cpp
        FindingCharacterSets.cpp
        FrontBackSlicing.cpp
        Hashing.cpp
        IdealFunctionParameter.cpp
        Indexing.cpp
        IteratingCharacters.cpp
        JoiningText.cpp
        LengthAttributes.cpp
        main.cpp
        PartialStringComparison.cpp
        RemoveCharacters.cpp
        SharedTools.hpp
        SliceAndKept.cpp
        SliceBoundaries.cpp
        StorageIdentifier.cpp
        SplittingText.cpp
        StdVsCoreCaseFold.cpp
        StdVsCoreEscape.cpp
        StdVsCoreSplitAndJoin.cpp
        StringViewDemos.hpp
        TestForCharacters.cpp
        TestStartsEndsAndContains.cpp
        TokenRuns.cpp
        TrimCharacterSet.cpp
        ValidateCharacterPolicy.cpp
        WholeStringComparison.cpp
)
