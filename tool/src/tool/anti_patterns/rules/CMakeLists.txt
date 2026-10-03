# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-tool-lib PRIVATE
        AnonymousNamespaceRule.cpp
        AnonymousNamespaceRule.hpp
        ApiDocumentationOnMemberImplementationRule.cpp
        ApiDocumentationOnMemberImplementationRule.hpp
        ErbslandCoreContainersStrictRule.cpp
        ErbslandCoreContainersStrictRule.hpp
        ForwardDeclarationRule.cpp
        ForwardDeclarationRule.hpp
        ImplementationInWrongUnitRule.cpp
        ImplementationInWrongUnitRule.hpp
        MisplacedUnittestMetadataRule.cpp
        MisplacedUnittestMetadataRule.hpp
        MissingApiDocumentationRule.cpp
        MissingApiDocumentationRule.hpp
        MissingDefaultGroupCommentRule.cpp
        MissingDefaultGroupCommentRule.hpp
        MultipleNamespacesRule.cpp
        MultipleNamespacesRule.hpp
        MultipleTypesInHeaderRule.cpp
        MultipleTypesInHeaderRule.hpp
        NamedConstantForRepeatedLiteralsRule.cpp
        NamedConstantForRepeatedLiteralsRule.hpp
        NamespaceInWrongUnitRule.cpp
        NamespaceInWrongUnitRule.hpp
        NamespaceNoiseRule.cpp
        NamespaceNoiseRule.hpp
        NestedNamespaceRule.cpp
        NestedNamespaceRule.hpp
        OversizedFileRule.cpp
        OversizedFileRule.hpp
        OversizedNestedTypeRule.cpp
        OversizedNestedTypeRule.hpp
        RegularStringLiteralRule.cpp
        RegularStringLiteralRule.hpp
        StaticCastVoidRule.cpp
        StaticCastVoidRule.hpp
        StaticGlobalObjectRule.cpp
        StaticGlobalObjectRule.hpp
        StaticOnlyClassRule.cpp
        StaticOnlyClassRule.hpp
        TypeInWrongUnitRule.cpp
        TypeInWrongUnitRule.hpp
        UnnecessaryTypeConstructionRule.cpp
        UnnecessaryTypeConstructionRule.hpp
        UnnecessaryUnitConversionRule.cpp
        UnnecessaryUnitConversionRule.hpp
)
