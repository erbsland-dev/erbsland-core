# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        ArgumentUnit.hpp
        ByteIndex.hpp
        ByteLength.hpp
        ByteOffset.hpp
        ByteRange.hpp
        ByteUnit.hpp
        CodeContinuousRange.hpp
        CodeLocation.hpp
        ColumnCount.hpp
        ColumnIndex.hpp
        ColumnOffset.hpp
        ColumnRange.hpp
        ColumnUnit.hpp
        CpIndex.hpp
        CpLength.hpp
        CpOffset.hpp
        CpRange.hpp
        CpUnit.hpp
        ElementCount.hpp
        ElementIndex.hpp
        ElementOffset.hpp
        ElementRange.hpp
        ElementUnit.hpp
        ExitCode.hpp
        IntegerAmount.hpp
        IntegerAmount_fwd.hpp
        IntegerUnit.hpp
        IntegerUnit_fwd.hpp
        IntegerUnitAmount.hpp
        IntegerUnitAmount_fwd.hpp
        IntegerUnitIndex.hpp
        IntegerUnitIndex_fwd.hpp
        IntegerUnitOffset.hpp
        IntegerUnitOffset_fwd.hpp
        IntegerUnitRange.hpp
        IntegerUnitRange_fwd.hpp
        LineCount.hpp
        LineIndex.hpp
        LineOffset.hpp
        LineRange.hpp
        LineUnit.hpp
        StdFormatForUnit.cpp
        StdFormatForUnit.hpp
        U16DataIndex.hpp
        U16DataLength.hpp
        U16DataOffset.hpp
        U16DataRange.hpp
        U16DataUnit.hpp
        Version.cpp
        Version.hpp
        VersionPart.hpp
        VersionRange.hpp
        VersionRange_fwd.hpp
        VersionUnit.hpp
)
