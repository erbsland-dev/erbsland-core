# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

target_sources(regex-api-profile PRIVATE
        BuiltInCatalog.cpp
        BuiltInCatalog.hpp
        BuiltInCatalog_fwd.hpp
        BuiltInConfiguration.hpp
        BuiltInConfiguration_fwd.hpp
        ConfigurationLoader.hpp
        ConfigurationLoader_fwd.hpp
        Execution.hpp
        Execution_fwd.hpp
        PreparedScenario.hpp
        PreparedScenario_fwd.hpp
        WorkloadRunner.hpp
        WorkloadRunner_fwd.hpp
)
