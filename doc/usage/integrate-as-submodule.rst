..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Usage; Git submodule
    single: CMake; erbsland::core
    single: Project layout

*********************************
Integrate Core as a Git Submodule
*********************************

The recommended way to consume Erbsland Core is to pin it as a Git submodule and build it with your application.
This gives your project an exact dependency revision and lets the compiler apply one consistent toolchain and build
configuration across application and library code.

Recommended Project Layout
==========================

Place all Erbsland libraries below one directory:

.. code-block:: none

    <project>
        ├── erbsland
        │   ├── core                    # Git submodule
        │   ├── <extension>             # Future Erbsland extension
        │   └── CMakeLists.txt          # Erbsland aggregation
        ├── <application-or-library>
        │   ├── src
        │   └── CMakeLists.txt
        └── CMakeLists.txt

Add Core from the project root:

.. code-block:: console

    $ mkdir erbsland
    $ git submodule add https://github.com/erbsland-dev/erbsland-core.git erbsland/core

Initialize all recorded dependencies after cloning a project:

.. code-block:: console

    $ git submodule update --init --recursive

Aggregate Erbsland Libraries
============================

The project root only needs to know about the ``erbsland`` directory and its own targets:

.. code-block:: cmake
    :caption: <project>/CMakeLists.txt

    cmake_minimum_required(VERSION 3.28)
    project(ExampleProject LANGUAGES CXX)

    add_subdirectory(erbsland)
    add_subdirectory(app)

The aggregation file owns the individual Erbsland subdirectories:

.. code-block:: cmake
    :caption: <project>/erbsland/CMakeLists.txt

    add_subdirectory(core EXCLUDE_FROM_ALL)
    # add_subdirectory(<extension> EXCLUDE_FROM_ALL)

This structure keeps the top-level project stable when extensions are added or removed.
Add Core before extensions because an extension may use Core's types and CMake target.

Link Your Target
================

Link the namespaced source target from your application or library:

.. code-block:: cmake
    :caption: <project>/app/CMakeLists.txt

    add_executable(example src/main.cpp)
    target_compile_features(example PRIVATE cxx_std_20)
    target_link_libraries(example PRIVATE erbsland::core)

The target provides Core's public include directories, C++20 requirement, and any public compile definitions selected by
the Core build configuration.

Include only public headers below ``<erbsland/...>``.
Use focused headers for individual APIs or domain headers such as ``<erbsland/all_path.hpp>`` when a source file works
with many related types.
Do not include files through ``src/erbsland`` or depend on ``impl`` headers.

Update Deliberately
===================

Update Core inside the submodule, test your application, and then commit the new submodule revision in the parent
repository:

.. code-block:: console

    $ git -C erbsland/core fetch
    $ git -C erbsland/core switch --detach <tested-revision>
    $ git add erbsland/core

Erbsland Core is currently alpha and may introduce intentional breaking API changes.
Review the :doc:`/addendum/changelog` and run your complete test suite before recording a new revision.

.. seealso::

    :doc:`build-configuration` describes the options that must be selected before ``add_subdirectory(erbsland)``.
