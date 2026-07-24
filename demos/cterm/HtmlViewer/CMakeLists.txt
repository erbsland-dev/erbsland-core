# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/data/demo-tea.html" _html_viewer_default_file)

erbsland_core_add_demo(html-viewer
        src/HtmlViewerApp.cpp
        src/HtmlViewerApp.hpp
        src/main.cpp
)

target_compile_definitions(html-viewer
        PRIVATE
        ERBSLAND_CORE_HTML_VIEWER_DEFAULT_FILE="${_html_viewer_default_file}"
)
