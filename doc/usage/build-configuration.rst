..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Usage; Build configuration
    single: CMake; Options
    single: Namespace; Configuration
    single: Unity build
    single: Precompiled headers

************************
Configure the Core Build
************************

Core adjusts its defaults depending on whether it is the top-level CMake project or embedded below another project.
An application that uses the recommended submodule layout therefore receives a small consumer build by default.

CMake Options
=============

.. list-table:: Core build options
    :header-rows: 1
    :widths: 38 12 12 38

    *   - Option
        - Top level
        - Embedded
        - Purpose
    *   - ``ERBSLAND_CORE_ENABLE_TESTS``
        - On
        - Off
        - Builds and registers the Core unit-test executable.
    *   - ``ERBSLAND_CORE_ENABLE_DEMOS``
        - On
        - Off
        - Configures the Core demo applications as explicit targets.
    *   - ``ERBSLAND_CORE_ENABLE_INTEROP_TESTS``
        - Off
        - Off
        - Configures standalone interoperability executables and their external-language counterparts.
    *   - ``ERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS``
        - On
        - Off
        - Uses Core's private standard-library precompiled header to reduce full-build time.
    *   - ``ERBSLAND_CORE_DO_NOT_FLATTEN_NS``
        - On
        - Off
        - Keeps APIs in their domain namespaces instead of importing them into ``erbsland``.
    *   - ``ERBSLAND_CORE_DEVELOPER_BUILD``
        - On
        - Off
        - Enables debug-only developer hooks and development build settings.

Options whose top-level and embedded defaults differ derive from CMake's ``PROJECT_IS_TOP_LEVEL`` value.
Set an option before ``add_subdirectory(erbsland)`` or pass it on the configure command line:

.. code-block:: console

    $ cmake -S . -B cmake-build -DERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS=ON

For a checked-in project policy, create the cache value before adding Erbsland:

.. code-block:: cmake

    set(ERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS ON CACHE BOOL "Enable Core precompiled headers")
    add_subdirectory(erbsland)

Tests and Demos
===============

Tests are part of a top-level Core development build and can be run through CTest:

.. code-block:: console

    $ cmake -S erbsland/core -B cmake-build-core -G Ninja -DCMAKE_BUILD_TYPE=Debug
    $ cmake --build cmake-build-core
    $ ctest --test-dir cmake-build-core --output-on-failure

Interoperability tests are intentionally separate from unit tests because they launch subprocesses.
Enable and run the compression, system, and network suites manually.
The compression and network suites require a Rust toolchain and build pinned counterparts through Cargo:

.. code-block:: console

    $ cmake -S erbsland/core -B cmake-build-interop -G Ninja -DCMAKE_BUILD_TYPE=Debug \
        -DERBSLAND_CORE_ENABLE_INTEROP_TESTS=ON
    $ cmake --build cmake-build-interop \
        --target erbsland-core-compression-interop erbsland-core-system-interop erbsland-core-network-interop
    $ cmake-build-interop/test/interop/compression/erbsland-core-compression-interop
    $ cmake-build-interop/test/interop/system/erbsland-core-system-interop
    $ cmake-build-interop/test/interop/network/erbsland-core-network-interop

Demos are configured but excluded from the default build.
Build all registered demos explicitly with:

.. code-block:: console

    $ cmake --build cmake-build-core --target erbsland-core-all-demos

Precompiled Headers and Unity Builds
====================================

Precompiled headers are a private build optimization and do not affect Core's public API.
Disable them for an unsupported or unusual compiler with ``-DERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS=OFF``.

When Core is top-level, developer mode is enabled, and ``CMAKE_UNITY_BUILD`` was not explicitly set, Core enables CMake
unity builds to reduce clean build time.
An embedded build never changes this setting automatically.
Select a conventional build with ``-DCMAKE_UNITY_BUILD=OFF`` or explicitly enable unity builds with
``-DCMAKE_UNITY_BUILD=ON``.

Core's unit tests and demos follow the selected unity-build mode.
The conventional CI workflow builds the unit tests without unity builds or precompiled headers to verify
translation-unit isolation.

Namespace Configuration
=======================

Regular consumer builds flatten domain namespaces into ``erbsland``.
This makes ``erbsland::text::String`` available as ``erbsland::String`` and provides the short alias ``el`` so user-code
can use ``el::String``.

Set ``ERBSLAND_CORE_DO_NOT_FLATTEN_NS`` to keep types only in their domain namespaces.
The CMake option publishes the corresponding compile definition through ``erbsland::core`` so Core and its consumers use
the same header configuration.

Two consumer compile definitions control only the short namespace alias:

``ERBSLAND_NO_SHORT_NAMESPACE``
    Disables the ``el`` alias.

``ERBSLAND_SHORT_NAMESPACE=<name>``
    Replaces ``el`` with another namespace alias.

Apply them to the consuming target when needed:

.. code-block:: cmake

    target_compile_definitions(example PRIVATE ERBSLAND_SHORT_NAMESPACE=myel)

These namespace options change source spelling, not the underlying C++ symbols or static-library ABI.
