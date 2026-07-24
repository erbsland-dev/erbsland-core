# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

target_sources(stream-file-profile PRIVATE
        ChunkSchedule.hpp
        ChunkSchedule_fwd.hpp
        ConfigurationTools.cpp
        ConfigurationTools.hpp
        Fixture.hpp
        Fixture_fwd.hpp
        PreparedScenario.hpp
        PreparedScenario_fwd.hpp
        ScenarioTemplate.hpp
        ScenarioTemplate_fwd.hpp
        Statistics.hpp
        Statistics_fwd.hpp
        ThroughputStatistics.hpp
        ThroughputStatistics_fwd.hpp
        WorkerFairness.hpp
        WorkerFairness_fwd.hpp
        WorkloadTools.hpp
)
