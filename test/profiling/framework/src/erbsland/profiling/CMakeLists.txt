# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-profiling PRIVATE
        AxisDefinition.cpp
        AxisDefinition.hpp
        AxisDefinition_fwd.hpp
        AxisSelection.hpp
        AxisSelection_fwd.hpp
        AxisValue.hpp
        AxisValue_fwd.hpp
        ConfigurationLoader.cpp
        ConfigurationLoader.hpp
        ConfigurationLoader_fwd.hpp
        Definitions.hpp
        FunctionalityDefinition.hpp
        FunctionalityDefinition_fwd.hpp
        MetricDefinition.hpp
        MetricDefinition_fwd.hpp
        ParameterDefinition.hpp
        ParameterDefinition_fwd.hpp
        ParameterSelection.hpp
        ParameterSelection_fwd.hpp
        ParameterType.hpp
        ParameterType_fwd.hpp
        ProfilingApplication.cpp
        ProfilingApplication.hpp
        ProfilingApplication_fwd.hpp
        ProfilingConfiguration.hpp
        ProfilingConfiguration_fwd.hpp
        ProfilingDefinition.cpp
        ProfilingDefinition.hpp
        ProfilingDefinition_fwd.hpp
        RunConfiguration.hpp
        RunConfiguration_fwd.hpp
        RunMode.hpp
        RunMode_fwd.hpp
        SampleMeasurement.hpp
        SampleMeasurement_fwd.hpp
        Scenario.hpp
        Scenario_fwd.hpp
        ScenarioWorkload.hpp
        ScenarioWorkload_fwd.hpp
        Seed.cpp
        Seed.hpp
        Statistics.cpp
        Statistics.hpp
        Statistics_fwd.hpp
        SteadyTimeSource.cpp
        SteadyTimeSource.hpp
        SteadyTimeSource_fwd.hpp
        SuiteDefinition.hpp
        SuiteDefinition_fwd.hpp
        TimeSource.cpp
        TimeSource.hpp
        TimeSource_fwd.hpp
        WorkerExecutionContext.hpp
        WorkerExecutionContext_fwd.hpp
        WorkerMeasurement.hpp
        WorkerMeasurement_fwd.hpp
        WorkerWorkload.hpp
        WorkerWorkload_fwd.hpp
        WorkloadRunner.cpp
        WorkloadRunner.hpp
        WorkloadRunner_fwd.hpp
)
