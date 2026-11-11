# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(pi_calculator PRIVATE
        main.cpp
        PiCalculatorApp.cpp
        PiCalculatorApp.hpp
)
target_sources(erbsland-core-pi-calculator-support PRIVATE
        DigitHistory.cpp
        DigitHistory.hpp
        DigitViewport.cpp
        DigitViewport.hpp
        PiDigitGenerator.cpp
        PiDigitGenerator.hpp
        PiSession.cpp
        PiSession.hpp
        PiView.cpp
        PiView.hpp
        PiWorker.cpp
        PiWorker.hpp
)
