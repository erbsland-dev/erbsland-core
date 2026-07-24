# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

target_sources(erbsland-profiling-unittest PRIVATE
        ConfigurationLoaderTest.cpp
        FakeScenarioWorkload.cpp
        FakeScenarioWorkload.hpp
        FakeScenarioWorkload_fwd.hpp
        FakeTimeSource.cpp
        FakeTimeSource.hpp
        FakeTimeSource_fwd.hpp
        FakeWorkerWorkload.cpp
        FakeWorkerWorkload.hpp
        FakeWorkerWorkload_fwd.hpp
        main.cpp
        ProfilingDefinitionTest.cpp
        SeedTest.cpp
        StatisticsTest.cpp
        WorkloadRunnerTest.cpp
)
