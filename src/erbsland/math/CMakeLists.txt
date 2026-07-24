# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        AnyIntegerTypes.hpp
        BoundedInteger.hpp
        ConstexprSaturatingMath.hpp
        IntegerBitOperations.hpp
        IntegerConversion.hpp
        IntegerMath.hpp
        IntegerRange.hpp
        IntegerTraits.hpp
        IntegerTypes.hpp
        SaturatingInteger.hpp
        SaturatingInteger_arithmetic.tpp
        SaturatingInteger_comparison.tpp
        SaturatingInteger_construction.tpp
        SaturatingInteger_conversion.tpp
        SaturatingInteger_free_operators.tpp
        SaturatingInteger_fwd.hpp
        SaturatingInteger_named.tpp
        SaturatingIntegerTypes.hpp
        SaturatingMath.hpp
        SaturatingMath_add.tpp
        SaturatingMath_cast.tpp
        SaturatingMath_divide.tpp
        SaturatingMath_modulo.tpp
        SaturatingMath_multiply.tpp
        SaturatingMath_subtract.tpp
        SignedMagnitude.hpp
)
