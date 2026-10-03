# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        ArgumentCount.hpp
        ArgumentIndex.hpp
        ArgumentUnit.hpp
        BuildNumber.hpp
        ByteCount.hpp
        ByteCount_fwd.hpp
        ByteIndex.hpp
        ByteIndex_fwd.hpp
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
        CpCount.hpp
        CpCount_fwd.hpp
        CpIndex.hpp
        CpIndex_fwd.hpp
        CpOffset.hpp
        CpRange.hpp
        CpRange_fwd.hpp
        CpUnit.hpp
        ExitCode.hpp
        IntegerAmount.hpp
        IntegerAmount_fwd.hpp
        IntegerUnit.hpp
        IntegerUnit_fwd.hpp
        IntegerUnitCount.hpp
        IntegerUnitCount_fwd.hpp
        IntegerUnitIndex.hpp
        IntegerUnitIndex_fwd.hpp
        IntegerUnitOffset.hpp
        IntegerUnitOffset_fwd.hpp
        IntegerUnitRange.hpp
        IntegerUnitRange_fwd.hpp
        ItemCount.hpp
        ItemCount_fwd.hpp
        ItemIndex.hpp
        ItemOffset.hpp
        ItemRange.hpp
        ItemUnit.hpp
        LineCount.hpp
        LineIndex.hpp
        LineOffset.hpp
        LineRange.hpp
        LineUnit.hpp
        Major.hpp
        Minor.hpp
        Revision.hpp
        StdFormat.hpp
        U16Count.hpp
        U16Count_fwd.hpp
        U16Index.hpp
        U16Offset.hpp
        U16Range.hpp
        U16Unit.hpp
        Version.cpp
        Version.hpp
        VersionComponent.hpp
        VersionComponent_fwd.hpp
        VersionPart.hpp
        VersionRange.hpp
        VersionRange_fwd.hpp
)
