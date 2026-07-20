# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        CharsConstraint.cpp
        CharsConstraint.hpp
        ConfKey.cpp
        ConfKey.hpp
        Constraint.cpp
        Constraint.hpp
        ConstraintHandlerContext.hpp
        DependencyDefinition.cpp
        DependencyDefinition.hpp
        DependencyMode.cpp
        DependencyMode.hpp
        DocumentValidator.cpp
        DocumentValidator.hpp
        DocumentValidator_constraints.cpp
        DocumentValidator_rules.cpp
        EqualsConstraint.cpp
        EqualsConstraint.hpp
        InConstraint.cpp
        InConstraint.hpp
        KeyConstraint.cpp
        KeyConstraint.hpp
        KeyDefinition.cpp
        KeyDefinition.hpp
        KeyIndex.cpp
        KeyIndex.hpp
        MatchesConstraint.cpp
        MatchesConstraint.hpp
        MinMaxConstraint.cpp
        MinMaxConstraint.hpp
        MultipleConstraint.cpp
        MultipleConstraint.hpp
        Rule.cpp
        Rule.hpp
        RuleMap.cpp
        RuleMap.hpp
        Rules.cpp
        Rules.hpp
        RulesBuilder.cpp
        RulesBuilder.hpp
        RulesConstants.hpp
        RulesDefinitionValidator.cpp
        RulesDefinitionValidator.hpp
        RulesFromDocument.cpp
        RulesFromDocument.hpp
        RulesFromDocument_constraintHandlers.cpp
        RulesFromDocument_nodes.cpp
        StringPartConstraint.cpp
        StringPartConstraint.hpp
        ValidationContext.hpp
        ValidationError.cpp
        ValidationError.hpp
        ValidationTarget.hpp
        VersionMask.cpp
        VersionMask.hpp
)
