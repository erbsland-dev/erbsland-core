# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

target_sources(regex-api-profile PRIVATE
        BuiltInCatalog.cpp
        BuiltInCatalog.hpp
        BuiltInCatalog_fwd.hpp
        BuiltInConfiguration.hpp
        BuiltInConfiguration_fwd.hpp
        ConfigurationLoader.cpp
        ConfigurationLoader.hpp
        ConfigurationLoader_fwd.hpp
        ConfigurationLoader_scenarios.cpp
        ConfigurationLoader_values.cpp
        Execution.cpp
        Execution.hpp
        Execution_fwd.hpp
        Execution_templates.tpp
        Execution_worker.cpp
        PreparedScenario.hpp
        PreparedScenario_fwd.hpp
        ScenarioTemplate.hpp
        WorkloadRunner.cpp
        WorkloadRunner.hpp
        WorkloadRunner_fwd.hpp
        WorkloadRunner_report.cpp
)
