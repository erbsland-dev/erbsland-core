..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Getting started; CMake
    single: CMake; erbsland::core

***********************
The CMake Configuration
***********************

Configure the Root Project
==========================

The root ``CMakeLists.txt`` connects the Erbsland libraries and your application.
Add Erbsland first so its targets exist when CMake configures ``elgrep``.

.. literalinclude:: files/CMakeLists.txt
    :language: cmake
    :caption: <project>/CMakeLists.txt

Add the Erbsland Aggregator
===========================

Create ``erbsland/CMakeLists.txt`` as the single place where the project registers Erbsland libraries.
When you add an extension later, add its subdirectory below ``core`` in this file.

.. literalinclude:: files/erbsland/CMakeLists.txt
    :language: cmake
    :caption: <project>/erbsland/CMakeLists.txt

``EXCLUDE_FROM_ALL`` keeps Core's own auxiliary targets out of regular application builds.
Core also detects that it is embedded and leaves its tests, demos, precompiled headers, and developer settings disabled
unless your project explicitly enables them.

Configure the Application Target
================================

The application CMake file declares the executable and links it to Core's source-integration target.

.. literalinclude:: files/elgrep/CMakeLists.txt
    :language: cmake
    :caption: <project>/elgrep/CMakeLists.txt

Linking ``erbsland::core`` supplies the public include path and the C++20 requirement.
The explicit ``target_compile_features`` line also documents the application's own language requirement for readers and
build tools.

.. button-ref:: 03-application-framework
    :ref-type: doc
    :color: success
    :align: center
    :expand:
    :class: sd-fs-5 sd-font-weight-bold sd-p-2 sd-my-4

    Build the Application Framework →
