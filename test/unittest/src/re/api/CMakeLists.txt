# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        DiagnosticsApiTest.cpp
        FeaturesAndSettingsTest.cpp
        InputBaseTest.cpp
        MatchTest.cpp
        RegExAlternativePrioritiesTest.cpp
        RegExAtomicGroupMinimalNestedTest.cpp
        RegExAtomicGroupMinimalTest.cpp
        RegExAtomicGroupNestedTest.cpp
        RegExAtomicGroupQuantifierTest.cpp
        RegExBase.hpp
        RegExCaptureGroupTest.cpp
        RegExCharClassBase.hpp
        RegExCharClassTest.cpp
        RegExCRLFTest.cpp
        RegExEmptyTest.cpp
        RegExErrorTest.cpp
        RegExFindAllCursorTest.cpp
        RegExGreedyExactQuantifierTest.cpp
        RegExGreedyMinimumQuantifierTest.cpp
        RegExGreedyPlusQuantifierTest.cpp
        RegExGreedyQuestionQuantifierTest.cpp
        RegExGreedyRangeQuantifierTest.cpp
        RegExGreedyStarQuantifierTest.cpp
        RegExLazyMinimumQuantifierTest.cpp
        RegExLazyPlusQuantifierTest.cpp
        RegExLazyQuestionQuantifierTest.cpp
        RegExLazyRangeQuantifierTest.cpp
        RegExLazyStarQuantifierTest.cpp
        RegExLongCharSequenceTest.cpp
        RegExNullCharacterTest.cpp
        RegExPossessiveExactQuantifierTest.cpp
        RegExPossessiveMinimumQuantifierTest.cpp
        RegExPossessivePlusQuantifierTest.cpp
        RegExPossessiveQuestionQuantifierTest.cpp
        RegExPossessiveRangeQuantifierTest.cpp
        RegExPossessiveStarQuantifierTest.cpp
        RegExRealWorldTest.cpp
        RegExReplaceAllTest.cpp
        RegExShortCharSequenceTest.cpp
        RegExStreamInputTest.cpp
        RegExUtf16Utf32Test.cpp
        RegExWordBoundaryTest.cpp
)
