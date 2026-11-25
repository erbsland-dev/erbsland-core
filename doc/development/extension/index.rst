..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Development; Writing an extension

*******************
Create an Extension
*******************

An Erbsland Core extension is a C++ library that gives one domain a home of its own.
You can develop and release it independently while sharing Core's types with the applications that use it.
Start with one public type and one test that uses it from outside the library.
That small test exercises the same header, CMake target, and compiled code that you will deliver to your readers.

Give the Domain a Name
======================

Choose a domain name that distinguishes your library from Core and the other extensions it will be used with.
The repository name is ``erbsland-core-<domain>``.
Carry the domain into the C++ namespace, the public include directory, and the CMake target.
Two libraries can then each publish a type called ``Reading`` without claiming the same header or C++ name.

.. erbsland-in-practice-begin:: A library for measurements

Use ``sample`` as a fictional domain.
Create an empty Git repository named ``erbsland-core-sample`` beside a development checkout of ``erbsland-core``.
Here is where the pieces will live as you work through this page and :doc:`maintenance`.

.. code-block:: text

    erbsland-core-sample/
    ├── CMakeLists.txt
    ├── setup.py
    ├── src/erbsland/sample/      # Declarations and compiled code
    ├── include/erbsland/         # Generated public headers
    ├── test/unittest/            # Tests that use the public target
    ├── tool/                     # Maintenance executable and its configuration
    └── doc/                      # Sphinx sources

A reading reports the unit of time used by the instrument.
Declare it in the domain namespace so an application can spell its name as ``erbsland::sample::Reading``.

.. code-block:: cpp
    :caption: src/erbsland/sample/Reading.hpp

    #pragma once

    #include <erbsland/String.hpp>

    namespace erbsland::sample {

    /// A reading from an instrument that measures elapsed time.
    class Reading final {
    public: // accessors
        /// Get the symbol for the unit used by this instrument.
        [[nodiscard]] auto unitSymbol() const -> text::String;
    };

    }

.. code-block:: cpp
    :caption: src/erbsland/sample/Reading.cpp

    #include "Reading.hpp"

    #include <erbsland/Literals.hpp>

    namespace erbsland::sample {

    using namespace text::literals;

    auto Reading::unitSymbol() const -> text::String {
        return "ms"_el;
    }

    }

An application compiling this declaration needs Core's string header.
An application calling the method also needs the compiled definition.
The next step gives it both through one target.

.. erbsland-in-practice-end::

Build Against One Copy of Core
==============================

An extension participates in two builds.
In your own checkout, you choose the Core sources so you can work on the two libraries side by side.
In an application, the parent project has already chosen and added Core.
Accepting its target lets every library use the same revision and build settings.

.. erbsland-in-practice-begin:: Define the sample library

Use the :download:`complete CMakeLists.txt <example/CMakeLists.txt>` at the repository root.
Its standalone section checks for a sibling Core checkout first and then for a checkout prepared by setup under
``test/erbsland-core``.
CMake adds the selected source directory once, with a separate binary directory.
If both are missing, configuration directs you to setup.
In an embedded build the file checks that the parent has already added ``erbsland::core``.

Both builds publish the same target.

.. literalinclude:: example/CMakeLists.txt
    :language: cmake
    :start-at: add_library(erbsland-core-sample STATIC)
    :end-at: add_subdirectory(src/erbsland/sample)

The helper links Core publicly and propagates its C++ requirements.
The extension adds its generated ``include`` tree to that public interface.
The alias ``erbsland::sample`` gives callers one name for the resulting interface.

.. code-block:: cmake
    :caption: src/erbsland/sample/CMakeLists.txt

    cmake_minimum_required(VERSION 3.28)

    target_sources(erbsland-core-sample PRIVATE
            Reading.cpp
            Reading.hpp
    )

.. erbsland-in-practice-end::

Test the Interface You Publish
==============================

Consumers see generated headers under ``include/erbsland``.
A wrapper gives each type a stable include path and forwards to the declaration you maintain under ``src``.
Commit the generated files with their sources so an application can build a pinned revision without installing your
maintenance tool.

.. erbsland-in-practice-begin:: Call the compiled method through its public header

From the extension root, add the test framework as a development submodule.

.. code-block:: console

    $ git submodule add https://github.com/erbsland-dev/erbsland-unittest.git test/erbsland-unittest

.. code-block:: cpp
    :caption: test/unittest/src/ReadingTest.cpp

    #include <erbsland/Literals.hpp>
    #include <erbsland/sample/Reading.hpp>
    #include <erbsland/unittest/UnitTest.hpp>

    using namespace erbsland::text::literals;

    TESTED_TARGETS(Reading)
    class ReadingTest final : public el::UnitTest {
    public:
        void testUnitSymbol() {
            REQUIRE_EQUAL(erbsland::sample::Reading{}.unitSymbol(), "ms"_el);
        }
    };

.. code-block:: cpp
    :caption: test/unittest/src/main.cpp

    #include <erbsland/unittest/UnitTest.hpp>

    auto main(int argc, char *argv[]) -> int {
        return erbsland::unittest::Controller::instance()->main(argc, argv);
    }

.. code-block:: cmake
    :caption: test/unittest/CMakeLists.txt

    add_executable(erbsland-sample-unittest src/main.cpp src/ReadingTest.cpp)
    target_link_libraries(erbsland-sample-unittest PRIVATE erbsland::sample)
    erbsland_unittest(TARGET erbsland-sample-unittest)
    add_test(NAME sample-reading COMMAND erbsland-sample-unittest --no-color)

The first time you build this test, it will need the wrapper ``<erbsland/sample/Reading.hpp>``.
Continue with :doc:`maintenance` to generate it, run the test, and publish the method in the reference.
Once the wrapper is in version control, a consuming project can link the library directly as described in
:doc:`/usage/extensions`.

.. erbsland-in-practice-end::

.. toctree::
    :maxdepth: 1

    maintenance
