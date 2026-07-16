..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Getting started; Project structure
    single: Git submodule

*********************
The Project Structure
*********************

Organize Erbsland Libraries Together
====================================

Erbsland Core is designed to live at ``<project>/erbsland/core``.
Future Erbsland extension libraries can then be placed next to it at ``<project>/erbsland/<extension>``.
Keeping these dependencies below one directory makes their ownership and CMake integration clear.

The completed tutorial project has this structure:

.. code-block:: none

    elgrep-project
        ├── erbsland
        │   ├── core                    # Erbsland Core Git submodule
        │   └── CMakeLists.txt          # Aggregates Erbsland libraries
        ├── elgrep
        │   ├── src
        │   │   ├── ElGrepApp.cpp
        │   │   ├── ElGrepApp.hpp
        │   │   └── main.cpp
        │   └── CMakeLists.txt
        └── CMakeLists.txt

The application has a small class of its own rather than placing all behavior in ``main.cpp``.
This separation keeps the program entry point simple and gives the application lifecycle a natural home.

Create the Project
==================

#.  Create and initialize the project repository:

    .. code-block:: console

        $ mkdir elgrep-project
        $ cd elgrep-project
        $ git init

#.  Add Erbsland Core at the recommended location:

    .. code-block:: console

        $ mkdir erbsland
        $ git submodule add https://github.com/erbsland-dev/erbsland-core.git erbsland/core

#.  Create the application source directory:

    .. code-block:: console

        $ mkdir -p elgrep/src

When somebody clones the project later, they can initialize every dependency with one command:

.. code-block:: console

    $ git submodule update --init --recursive

The recorded submodule revision is part of your project.
This lets you update Core deliberately, review its changes, and test the new revision before committing it.

.. button-ref:: 02-cmake-configuration
    :ref-type: doc
    :color: success
    :align: center
    :expand:
    :class: sd-fs-5 sd-font-weight-bold sd-p-2 sd-my-4

    Configure the Project with CMake →
