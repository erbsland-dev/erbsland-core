..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Usage; Extensions
    single: CMake; Extension packages

****************
Use an Extension
****************

An extension gives your application a library for a particular domain.
It shares Core's types, so a value returned by an extension can go straight into code that already uses Core.
For example, an instrument could report its unit of measurement as a Core string.

The recommended way to bring an extension into your build is to pin its sources alongside Core.
If the extension's publisher provides a shared library with a CMake package, you can also use that installation.
In either case your application includes the extension's public headers and links its CMake target.

Build the Sources Together
==========================

Give each dependency its own checkout under the application's ``erbsland`` directory.
Keep Core at ``erbsland/core`` and each extension at ``erbsland/<domain>``.
The parent repository records the revisions that work together, and CMake builds them with the application's toolchain.

.. erbsland-in-practice-begin:: Two submodules and one application

Suppose a fictional extension named ``sample`` provides measurements.
Add its repository, ``erbsland-core-sample``, as a submodule at ``erbsland/sample`` alongside the
:doc:`Core submodule <integrate-as-submodule>`.
After cloning the application, ``git submodule update --init --recursive`` checks out the recorded revisions.

.. code-block:: text

    application/
    ├── CMakeLists.txt
    ├── main.cpp
    └── erbsland/
        ├── CMakeLists.txt
        ├── core/
        └── sample/

.. code-block:: cmake
    :caption: erbsland/CMakeLists.txt

    add_subdirectory(core EXCLUDE_FROM_ALL)
    add_subdirectory(sample EXCLUDE_FROM_ALL)

CMake processes these lines in order.
By the time it reaches ``sample``, the target ``erbsland::core`` exists.
Every extension you add here can link to that same target.
``EXCLUDE_FROM_ALL`` lets the application target determine which libraries the build needs.

.. code-block:: cmake
    :caption: CMakeLists.txt

    cmake_minimum_required(VERSION 3.28)
    project(Measurements LANGUAGES CXX)

    add_subdirectory(erbsland)
    add_executable(measurements main.cpp)
    target_link_libraries(measurements PRIVATE erbsland::sample)

The top-level file names the library the application uses.
If ``sample`` later needs another extension, you can add that dependency before ``sample`` in
``erbsland/CMakeLists.txt`` and keep the application's link line.

.. erbsland-in-practice-end::

Use the Public Interface
========================

The target carries the include paths, compiler requirements, and public Core dependency.
Choose the public header for the type your application uses.
An extension also provides a domain umbrella, such as ``<erbsland/all_sample.hpp>``, for code that uses several of its
types together.

.. erbsland-in-practice-begin:: Display the unit of a reading

.. code-block:: cpp
    :caption: main.cpp

    #include <erbsland/sample/Reading.hpp>
    #include <erbsland/StringConverter.hpp>

    #include <iostream>

    auto main() -> int {
        const auto reading = erbsland::sample::Reading{};
        std::cout << erbsland::text::StringConverter{reading.unitSymbol()}.toStdString() << '\n';
    }

Configure with ``cmake -S . -B build`` and build with ``cmake --build build``.
The executable prints ``ms``.
The header for ``Reading`` comes from the extension and the string converter comes from Core.
Linking ``erbsland::sample`` makes both available.

.. erbsland-in-practice-end::

.. _usage-installed-shared-extension:

Use an Installed Shared Extension
=================================

Some publishers distribute a compiled extension together with its headers and a CMake package.
Install the development package for your platform and toolchain, including the compatible Core version specified by the
publisher.
Core itself currently installs as a :doc:`static library <install-static-library>`.
Look up the package name and the imported target name in the extension's documentation.

.. erbsland-in-practice-begin:: Use the packaged measurements library

Suppose the publisher of the fictional example supplies a package named ``erbsland-core-sample`` that finds its Core
dependency and exports ``erbsland::sample``.
The application can now obtain that target with ``find_package()``.

.. code-block:: cmake
    :caption: CMakeLists.txt for the installed package

    cmake_minimum_required(VERSION 3.28)
    project(Measurements LANGUAGES CXX)

    find_package(erbsland-core-sample CONFIG REQUIRED)
    add_executable(measurements main.cpp)
    target_link_libraries(measurements PRIVATE erbsland::sample)

If you installed both packages under a custom prefix, point CMake at that prefix when you configure the application.

.. code-block:: console

    $ cmake -S . -B build-installed -DCMAKE_PREFIX_PATH=/absolute/path/to/prefix
    $ cmake --build build-installed

CMake reads the package configuration to locate the headers and the library for your build.
You can compile the same ``main.cpp`` shown above.

.. erbsland-in-practice-end::

When you distribute the executable, the operating system must also be able to load the extension's shared library.
``CMAKE_PREFIX_PATH`` served its purpose during configuration.
The runtime loader uses the search paths on the machine that runs your program.
Follow the publisher's deployment instructions for that platform.
Windows applications commonly ship the DLL beside the executable.
A macOS app can bundle the dylib and refer to it through a runtime search path.
A Linux package can declare a dependency on the distribution's shared-library package.
Run the deployed executable in its intended layout to check that the loader can find the library.
