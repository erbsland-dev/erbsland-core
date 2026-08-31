# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(logging_topics)
target_sources(logging_topics PRIVATE
        ApplicationLogging.cpp
        ConsoleWriterOptions.cpp
        ConsoleWriters.cpp
        ConfigurationReplacement.cpp
        FileWriterOptions.cpp
        FileWriters.cpp
        LastErrorsWriter.cpp
        LineFirstLineTruncation.cpp
        LineFormats.cpp
        LineLevelFormats.cpp
        LineMessageLimits.cpp
        LineNameFormats.cpp
        LineNameLimits.cpp
        LinePatterns.cpp
        LineTimestampZones.cpp
        LineTotalLimits.cpp
        LineTruncationMarks.cpp
        LoadApplicationLogConfiguration.cpp
        LoadStandaloneLogConfiguration.cpp
        LogLevels.cpp
        LoggingTopicsDemos.hpp
        LogStreams.cpp
        main.cpp
        ManagerByteCapacity.cpp
        ManagerDemoWriters.hpp
        ManagerEntryCapacity.cpp
        ManagerErrorByteReserve.cpp
        ManagerErrorEntryReserve.cpp
        ManagerMessageSize.cpp
        ManagerOptions.cpp
        ManagerShutdownTimeout.cpp
        ManagerStatistics.cpp
        ManualConfiguration.cpp
        MultipleLogStreams.cpp
        StoredLogStream.cpp
        SyslogWriterOptions.cpp
        SyslogWriters.cpp
        TraceSectionActivation.cpp
        TraceSectionGrouping.cpp
        TraceSectionGuard.cpp
        TraceSectionReplacement.cpp
        TraceSections.cpp
        WriterRouting.cpp
        WriterSetup.cpp
)
