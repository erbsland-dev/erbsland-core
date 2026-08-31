# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        all.hpp
        ConsoleLogWriter.cpp
        ConsoleLogWriter.hpp
        ConsoleLogWriter_fwd.hpp
        ConsoleLogWriterOptions.cpp
        ConsoleLogWriterOptions.hpp
        FileLogWriter.cpp
        FileLogWriter.hpp
        FileLogWriterOptions.cpp
        FileLogWriterOptions.hpp
        LastErrorsLogWriter.cpp
        LastErrorsLogWriter.hpp
        LastErrorsLogWriter_fwd.hpp
        LogConfiguration.cpp
        LogConfiguration.hpp
        LogConfigurationParser.cpp
        LogConfigurationParser.hpp
        LogEntry.cpp
        LogEntry.hpp
        LogEntry_fwd.hpp
        LogFileMode.cpp
        LogFileMode.hpp
        LogFileRotation.cpp
        LogFileRotation.hpp
        LogLevel.cpp
        LogLevel.hpp
        LogLevelFormat.cpp
        LogLevelFormat.hpp
        LogLine.cpp
        LogLine.hpp
        LogLine_fwd.hpp
        LogLineFormat.cpp
        LogLineFormat.hpp
        LogLinePart.cpp
        LogLinePart.hpp
        LogLineSegment.hpp
        LogManager.cpp
        LogManager.hpp
        LogManager_fwd.hpp
        LogManagerOptions.cpp
        LogManagerOptions.hpp
        LogManagerStatistics.hpp
        LogMessageTruncation.cpp
        LogMessageTruncation.hpp
        LogNameFormat.cpp
        LogNameFormat.hpp
        LogPath.cpp
        LogPath.hpp
        LogStream.cpp
        LogStream.hpp
        LogStream_fwd.hpp
        LogTimestampZone.cpp
        LogTimestampZone.hpp
        LogTraceSection.cpp
        LogTraceSection.hpp
        LogWriter.cpp
        LogWriter.hpp
        LogWriter_fwd.hpp
        LogWriterFilter.cpp
        LogWriterFilter.hpp
        SyslogLogWriter.cpp
        SyslogLogWriter.hpp
        SyslogLogWriterOptions.cpp
        SyslogLogWriterOptions.hpp
        SyslogTransport.cpp
        SyslogTransport.hpp
)
