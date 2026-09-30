..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Release Packages
    single: Packaging; Getting Started

*************************
Creating Release Packages
*************************

A finished executable is rarely the whole application.
It may need DLLs or frameworks, a license, and a configuration file before someone else can run it.
Erbsland Core can collect these pieces into a ZIP during CMake installation, so the archive you share is built from the
same release configuration you just tested.
This page gets a small application packaged on Windows or macOS.
The following pages explain :doc:`release_package_cmake`, :doc:`release_package_configuration`, and
:doc:`release_package_signing` when you need more control.

What a Release Package Contains
===============================

For each registered package, the tool stages the named executable and its non-system runtime libraries in a temporary
directory, creates a ZIP, and places it in the project's ``packages`` directory.
On Windows, DLLs sit beside the executable so the Windows loader can find them.
On macOS, the default is an ``.app`` bundle: libraries and frameworks go into ``Contents/Frameworks``, and the tool
repairs their load paths before archiving the bundle.
The exact libraries depend on what your executable links at build time.

Signing is a separate choice.
On Windows, an Authenticode signature identifies the publisher of the executable and staged DLLs; a timestamp lets the
signature remain useful after the signing certificate expires.
On macOS, Developer ID signing identifies a directly distributed app, while Apple's notarization service checks a
submitted signed app and issues a ticket that can be attached to it.
The tool can perform both steps when you provide signing credentials.
You can first build an unsigned package, then set up :doc:`release_package_signing` for distribution.

Packaging is supported on Windows and macOS.
On Linux, the same CMake call prints a warning during configuration and registers no package rule.
Your executable and any separate Linux install rules still work, so a cross-platform project can keep one
``erbsland_core_package()`` call.

Register Your First Package
===========================

Suppose ``erbsland/core`` is a Git submodule in your project, as described in
:doc:`/usage/integrate-as-submodule`.
After ``add_subdirectory()``, the application setup helper and the package function are available in the same CMake
tree.
This complete root ``CMakeLists.txt`` registers one package named ``harbor``:

.. code-block:: cmake

    cmake_minimum_required(VERSION 3.28)
    project(harbor VERSION 1.4.0 LANGUAGES CXX)

    add_subdirectory(erbsland/core)

    add_executable(harbor_app src/main.cpp)
    erbsland_core_setup_application(TARGET harbor_app)

    erbsland_core_package(TARGET harbor_app NAME harbor)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        install(TARGETS harbor_app RUNTIME DESTINATION bin)
    endif()

The ``project()`` version supplies the package version by default.
``TARGET`` names an executable target that already exists at configure time; ``NAME`` becomes the package name.
On Linux, CMake warns that no release ZIP is created; ``harbor_app`` remains a normal build target.
The separate ``install(TARGETS ...)`` rule installs the executable under the chosen prefix's ``bin`` directory.
Your application can add further Linux install rules for its data and documentation.

On Windows or macOS, build and install the release configuration:

.. code-block:: shell

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target harbor_app --config Release
    cmake --install build --config Release --component Package-harbor

The install component selects just this package.
The last command builds the package utility automatically, but the application executable must already have been built.
Without ``--component``, CMake runs all install rules, including every registered package and your ordinary install
rules.
``Release`` and ``RelWithDebInfo`` create ZIPs; an install of another configuration warns and skips packaging.
Package output does not follow ``CMAKE_INSTALL_PREFIX``: it goes into ``packages`` below the root source directory.

On Linux, developers commonly build from source and install into a chosen prefix:

.. code-block:: shell

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    cmake --install build --prefix "$HOME/.local"

This example puts the executable in ``$HOME/.local/bin``.
If you instead prepare a package for a particular Linux distribution, its packaging script can stage the same CMake
install rules before assembling the distribution's native package:

.. code-block:: shell

    DESTDIR="$PWD/stage" cmake --install build --prefix /usr

The staging tree then contains ``stage/usr/bin/harbor_app``.
Your distribution packaging script decides how to turn that tree into its final package.
The ``erbsland_core_package()`` call does not take over either Linux installation path.

With no ``package.elcl`` file, the resulting archive is named ``harbor-1.4.0-windows-x64.zip`` on a Windows x64 build or
``harbor-1.4.0-macos-arm64.zip`` on a macOS arm64 build.
Architecture comes from the build configuration, so yours may differ.
The following trees show the shape of each ZIP; runtime library names are examples:

.. code-block:: text

    harbor-1.4.0-windows-x64.zip
    └── harbor-1.4.0/
        ├── harbor_app.exe
        └── harbor-runtime.dll       # Only when this non-system DLL is required.

    harbor-1.4.0-macos-arm64.zip
    └── harbor-1.4.0/
        └── harbor_app.app/
            └── Contents/
                ├── Info.plist
                ├── MacOS/harbor_app
                └── Frameworks/       # Populated when runtime libraries are needed.

The directory inside the ZIP and the ZIP basename have separate formats.
This matters when you want a stable archive name but still want a versioned directory after extraction.

Add Your Own Files
==================

Place ``package.elcl`` beside the root ``CMakeLists.txt`` to add a license and default settings:

.. code-block:: text

    [main]

    *[main.files]*
    path: "LICENSE.txt"
    target: "docs"

The file is optional; the CMake call does not change when you add it.
The source path is relative to ``package.elcl``.
After the next release install, both platform archives also contain ``harbor-1.4.0/docs/LICENSE.txt``.
You can add more file entries, change archive names, set platform overrides, or choose another version source in
:doc:`release_package_configuration`.
If you have several executables or want separate packages, :doc:`release_package_cmake` shows how registration and
install components fit together.
When you are ready to distribute a trusted build, continue with :doc:`release_package_signing`.
