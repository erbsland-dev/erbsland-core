..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Requirements

************
Requirements
************

Using Erbsland Core
===================

To build an application with Erbsland Core you need:

*   A compiler and standard library with C++20 support.

    *   MSVC 19.50 or newer (older versions have unresolved issues compiling modern C++20 code)
    *   GCC 14 or newer, with libstdc++ 14 or newer.
    *   LLVM Clang 17 or newer, with libc++ 17 or newer, or libstdc++ 14 or newer.

*   CMake 3.28 or newer.
*   Git when integrating Core as the recommended Git submodule.
*   A supported desktop or server platform: Linux, macOS, or Windows.

Core has no required third-party runtime or library dependencies beyond the C++ standard library.

Ninja is optional.
The documentation uses it in commands because it is fast and behaves consistently across the supported platforms, but
another CMake generator can be used instead.
Omit ``-G Ninja`` when you want CMake to select its default generator.

Developing Erbsland Core
========================

Contributors who build Core itself additionally need:

*   Python 3.14 for repository setup, test metadata generation, and Python tests.
*   The recursively initialized unit-test submodule.
*   The Python development packages pinned in ``requirements-dev.txt``.
*   Ninja for the documented development build.

Building the Sphinx documentation also requires the packages pinned in ``doc/requirements-doc.txt`` and a working
Doxygen installation.
Follow :doc:`/development/setup` to prepare a checkout, install these dependencies, and build and test the library.
See :doc:`/usage/build-configuration` for all supported Core build options.
