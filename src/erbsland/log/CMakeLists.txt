# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)
add_subdirectory(line)

target_sources(erbsland-core PRIVATE
        all.hpp
        ConsoleLogWriterOptions.cpp
        ConsoleLogWriterOptions.hpp
        ConsoleLogWriterOptions_fwd.hpp
        FileLogWriterOptions.cpp
        FileLogWriterOptions.hpp
        FileLogWriterOptions_fwd.hpp
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
        LogLineFormat.cpp
        LogLineFormat.hpp
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
        SyslogLogWriterOptions.cpp
        SyslogLogWriterOptions.hpp
        SyslogLogWriterOptions_fwd.hpp
        SyslogTransport.cpp
        SyslogTransport.hpp
)
