# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(option_help_documents)
target_sources(option_help_documents PRIVATE
        main.cpp
)

erbsland_core_display_text(TARGET option_help_documents FILE DisplayTextDemo.elcl NAMESPACE demo)
