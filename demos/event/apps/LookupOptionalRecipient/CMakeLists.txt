# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(lookup_optional_recipient)
target_sources(lookup_optional_recipient PRIVATE
        LookupOptionalRecipientApp.hpp
        LookupProgress.hpp
        main.cpp
)
