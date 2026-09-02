# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        ConfKeyTest.cpp
        DependencyModeTest.cpp
        KeyIndexTest.cpp
        RuleTypeTest.cpp
        VersionMaskTest.cpp
        VrAlternativesTest.cpp
        VrBase.hpp
        VrBuilderApiTest.cpp
        VrBuilderParityTest.cpp
        VrCharsTest.cpp
        VrContainsTest.cpp
        VrDefaultsAndOptionalityTest.cpp
        VrDependenciesTest.cpp
        VrDocumentModelTest.cpp
        VrEndsTest.cpp
        VrEqualsTest.cpp
        VrEvaluationOrderTest.cpp
        VrInTest.cpp
        VrKeysAndReferencesTest.cpp
        VrListsTest.cpp
        VrManualConstructionTest.cpp
        VrMatrixTest.cpp
        VrMaximumTest.cpp
        VrMinimumTest.cpp
        VrMultipleTest.cpp
        VrNodeRulesDefinitionTest.cpp
        VrPublicApiTest.cpp
        VrReservedNamesTest.cpp
        VrStartsTest.cpp
        VrSubBranchValidationTest.cpp
        VrTemplatesTest.cpp
        VrVariableNamesTest.cpp
        VrVersionTest.cpp
)
