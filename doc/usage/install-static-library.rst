..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Usage; Static library
    single: Installation
    single: CMake; find_package
    single: ErbslandDEV::erbsland-core

********************************
Install Core as a Static Library
********************************

Erbsland Core always builds as a static library.
Installing it separately is useful for package-building workflows or when several controlled projects share one exact
Core revision.

.. note::

    We strongly recommend to :doc:`integrate Erbsland Core as a submodule <integrate-as-submodule>`, as it remains
    simpler because source and application revisions stay together.

Build and Install
=================

Clone the chosen revision and configure a consumer-oriented Release build:

.. code-block:: console

    $ git clone https://github.com/erbsland-dev/erbsland-core.git
    $ cmake -S erbsland-core -B erbsland-core-build -G Ninja \
          -DCMAKE_BUILD_TYPE=Release \
          -DERBSLAND_CORE_ENABLE_TESTS=OFF \
          -DERBSLAND_CORE_ENABLE_DEMOS=OFF \
          -DERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS=ON \
          -DERBSLAND_CORE_DO_NOT_FLATTEN_NS=OFF \
          -DERBSLAND_CORE_DEVELOPER_BUILD=OFF
    $ cmake --build erbsland-core-build
    $ cmake --install erbsland-core-build --prefix "$PWD/erbsland-core-install"

Disabling ``ERBSLAND_CORE_DO_NOT_FLATTEN_NS`` gives installed consumers the regular flattened namespace and ``el``
alias.
Developer mode is disabled so the installed binary does not expose development-only hooks.

The selected prefix receives the static library, public headers, the package configuration, and the Git-version CMake
helper.

Consume the Package
===================

Use CMake's config-package mode and configure the executable with Core's application helper:

.. code-block:: cmake
    :caption: <consumer>/CMakeLists.txt

    cmake_minimum_required(VERSION 3.28)
    project(ExampleConsumer LANGUAGES CXX)

    find_package(erbsland-core CONFIG REQUIRED)

    add_executable(example src/main.cpp)
    erbsland_core_setup_application(TARGET example)

Point CMake to a custom installation prefix when configuring the consumer:

.. code-block:: console

    $ cmake -S . -B build -G Ninja \
          -DCMAKE_PREFIX_PATH=/absolute/path/to/erbsland-core-install
    $ cmake --build build

Both setup helpers select the available Core target automatically.
Use ``erbsland_core_setup_static_library(TARGET example_library)`` for a static-library target, whether Core is
integrated from source or consumed as an installed package.
