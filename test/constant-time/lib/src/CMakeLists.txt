# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(cases)

target_sources(erbsland-constant-time PRIVATE
        BackendInfo.hpp
        Checksum.cpp
        Checksum.hpp
        Evidence.hpp
        Fixture.hpp
        Moments.cpp
        Moments.hpp
        Outcome.hpp
        PreparedCase.hpp
        Registration.hpp
        Registry.cpp
        Registry.hpp
        Registry_authentication.cpp
        Registry_comparisons.cpp
        Registry_signing.cpp
        Registry_symmetric.cpp
        Registry_validation.cpp
        Result.cpp
        Result.hpp
        Runner.cpp
        Runner.hpp
        RunOptions.hpp
        RunResults.cpp
        RunResults.hpp
        SelfTest.cpp
        SelfTest.hpp
        Statistics.cpp
        Statistics.hpp
        TestCase.hpp
        TestMetadata.hpp
        WelchTest.cpp
        WelchTest.hpp
)
