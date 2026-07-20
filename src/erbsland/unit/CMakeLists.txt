# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        ArgumentUnit.hpp
        ByteIndex.hpp
        ByteIndex_fwd.hpp
        ByteLength.hpp
        ByteLength_fwd.hpp
        ByteOffset.hpp
        ByteRange.hpp
        ByteRange_fwd.hpp
        ByteUnit.hpp
        CodeContinuousRange.hpp
        CodeLocation.cpp
        CodeLocation.hpp
        ColumnCount.hpp
        ColumnIndex.hpp
        ColumnOffset.hpp
        ColumnRange.hpp
        ColumnUnit.hpp
        CpIndex.hpp
        CpIndex_fwd.hpp
        CpLength.hpp
        CpLength_fwd.hpp
        CpOffset.hpp
        CpRange.hpp
        CpRange_fwd.hpp
        CpUnit.hpp
        ElementCount.hpp
        ElementCount_fwd.hpp
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
        StdFormatForUnit.hpp
        U16DataIndex.hpp
        U16DataLength.hpp
        U16DataLength_fwd.hpp
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
