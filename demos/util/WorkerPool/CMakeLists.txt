# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(worker_pool)
target_sources(worker_pool PRIVATE
        EventDelivery.cpp
        main.cpp
        Procedural.cpp
        WorkerPoolDemos.hpp
)
