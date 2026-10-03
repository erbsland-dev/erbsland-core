# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        ActiveLogConfiguration.cpp
        ActiveLogConfiguration.hpp
        ActiveLogConfiguration_fwd.hpp
        ActiveLogWriter.hpp
        ConsoleLogWriter.cpp
        ConsoleLogWriter.hpp
        ConsoleLogWriter_fwd.hpp
        FileLogWriter.cpp
        FileLogWriter.hpp
        LastErrorsLogWriter.cpp
        LastErrorsLogWriter.hpp
        LastErrorsLogWriter_fwd.hpp
        LogConfigurationRules.cpp
        LogConfigurationRules.hpp
        LogLineFormatter.cpp
        LogLineFormatter.hpp
        LogLineFormatter_fwd.hpp
        LogManagerData.cpp
        LogManagerData.hpp
        LogManagerData_fwd.hpp
        LogWriterBinding.hpp
        SyslogLogWriter.cpp
        SyslogLogWriter.hpp
        ValidateTraceSection.hpp
)
