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

The root ``CMakeLists.txt`` connects the Erbsland Core libraries and your application.
Add Erbsland Core first so its targets exist when CMake configures ``elgrep``.

.. literalinclude:: files/CMakeLists.txt
    :language: cmake
    :caption: <project>/CMakeLists.txt

Add the Erbsland Core Aggregator
===========================

Create ``erbsland/CMakeLists.txt`` as the single place where the project registers Erbsland Core libraries.
When you add an extension later, add its subdirectory below ``core`` in this file.

.. literalinclude:: files/erbsland/CMakeLists.txt
    :language: cmake
    :caption: <project>/erbsland/CMakeLists.txt

``EXCLUDE_FROM_ALL`` keeps Core's own auxiliary targets out of regular application builds.
Core also detects that it is embedded and leaves its tests, demos, precompiled headers, and developer settings disabled
unless your project explicitly enables them.

Configure the Application Target
================================

The application CMake file declares the executable and passes it to Core's application setup helper.

.. literalinclude:: files/elgrep/CMakeLists.txt
    :language: cmake
    :caption: <project>/elgrep/CMakeLists.txt

``erbsland_core_setup_application()`` links the available Core target, enables C++20, disables C++ module scanning for
the target, and selects UTF-8 source and execution character sets on MSVC.
The helper works with both source integration and an installed Core package, so the application CMake file does not need
to choose between their different target names.

.. button-ref:: 03-application-framework
    :ref-type: doc
    :color: success
    :align: center
    :expand:
    :class: sd-fs-5 sd-font-weight-bold sd-p-2 sd-my-4

    Build the Application Framework →
