..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***********************************
Build and Test the Maintenance Tool
***********************************

This is an optional step after :doc:`setting up the repository <../setup>`.
Follow it when you want to change the maintenance tool's C++ implementation or add a new command.
To run existing commands while working on the library, go to :doc:`../tool_usage/usage`.

The repository setup already provides the dependencies and a configured Debug build.
Tool development uses that same build; no second checkout or Python environment is needed.

.. _development-tool-setup-build-the-version-you-are-developing:

Build the Version You Are Developing
====================================

From the repository root, build the Debug executable and its tests:

.. code-block:: console

    cmake --build cmake-build-debug --target erbsland-core-tool erbsland-core-tool-unittest

Run that executable directly when trying your changes:

.. code-block:: console

    cmake-build-debug/tool/erbsland-core-tool --help

The root shortcut still runs the installed Release copy.
Keeping these executables separate lets you use the installed tool for library maintenance while trying changes in the
Debug build.
Re-running ``setup.py`` rebuilds and installs the Release copy when you want to update it.
That copy links Core and the tool implementation statically and uses the normal platform runtime.

If you previously disabled tools or tests in your CMake cache, enable ``ERBSLAND_CORE_ENABLE_TOOLS`` and
``ERBSLAND_CORE_ENABLE_TESTS`` before building these targets.
Both are enabled by default for a fresh top-level development build.

.. _development-tool-setup-run-the-tool-tests:

Run the Tool Tests
==================

After building the Debug targets, run the ordinary tool tests:

.. code-block:: console

    cmake-build-debug/tool/test/unittest/erbsland-core-tool-unittest

Include the exhaustive Unicode comparison when changing IDNA derivation or data:

.. code-block:: console

    cmake-build-debug/tool/test/unittest/erbsland-core-tool-unittest +tag:FullRun

The Python tests cover setup with mocked commands and run the Debug executable to check startup, file access, and output
preservation:

.. code-block:: console

    .venv/bin/python3 -m unittest discover -s test/python

On Windows, use the corresponding ``.exe`` executables and ``.venv\Scripts\python.exe`` for Python commands.

Read :doc:`framework` for an overview, then follow :doc:`write_command` or :doc:`write_data_source`.
Consult :doc:`../tool_reference/framework_reference` when changing shared behavior.
Before committing, run the repository checks described in :doc:`../setup`.
