..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Adding Git Version
    single: Application
    single: CMake; erbsland_core_add_git_version
    single: Git version
    single: Version
    single: Version Tags
    single: Subclass Pattern
    single: Direct Application Pattern
    single: CMake Setup
    single: erbsland_core_add_git_version
    single: Application
    single: setMainFn
    single: Build version
    single: Version metadata

******************************************
Adding Git Version Information to Your App
******************************************

Erbsland Core provides a small CMake helper that generates a source file containing version information from your Git
repository.

This allows an application to report its own version without introducing generated include directories, generated public
headers, or additional build-time dependencies throughout the code base.

The helper is intentionally designed for quiet and efficient builds.
It tracks the current Git ``HEAD``, the active branch reference, a submodule or worktree ``.git`` file, and
``packed-refs``.
It does not track the repository's dirty state and does not recursively scan all references.
This keeps dependency tracking predictable while still updating the generated version whenever the effective repository
version changes.

If Git is unavailable, the source tree does not contain a ``.git`` entry, or the version information cannot be parsed,
all generated values fall back to ``0.0.0.0``.

Version Tags
============

The helper derives version information from Git tags.
Use stable release tags in one of the following forms:

.. code-block:: text

    v1.2.3
    1.2.3

Internally, the helper runs:

.. code-block:: text

    git describe --tags --long --always

The latest reachable release tag provides the major, minor, and revision version numbers.
The number of commits since that tag becomes the build number.
The complete ``git describe`` output is also available as a version text string.

By default, templates use the variable prefix ``ERBSLAND_GIT_VERSION``:

.. code-block:: cpp

    @ERBSLAND_GIT_VERSION_MAJOR@
    @ERBSLAND_GIT_VERSION_MINOR@
    @ERBSLAND_GIT_VERSION_REVISION@
    @ERBSLAND_GIT_VERSION_BUILD@
    @ERBSLAND_GIT_VERSION_TEXT@

You can change this prefix with ``VARIABLE_PREFIX`` when a target requires multiple independently generated version
sources.

Subclass Pattern
================

This pattern is useful when your application already derives from
:cpp:class:`Application <erbsland::core::Application>` to provide custom
initialization, command-line handling, or other startup logic.

Add two static accessors to your application class:

.. code-block:: cpp

    #pragma once

    #include <erbsland/Application.hpp>
    #include <erbsland/StringView.hpp>
    #include <erbsland/Version.hpp>

    class MyApplication final : public el::Application {
    public:
        using el::Application::Application;

        [[nodiscard]] static auto appVersion() noexcept -> el::Version;
        [[nodiscard]] static auto appVersionText() noexcept -> el::StringView;

        void initialize() override;
    };

Implement ``initialize()`` as usual and use the generated version information to populate the application metadata.

.. code-block:: cpp

    #include "MyApplication.hpp"

    void MyApplication::initialize() {
        using namespace el::text::literals;

        el::Application::initialize();
        info().setApplicationName("MyApp"_el);
        info().setApplicationVersion(appVersion());
    }

Next, create ``MyApplication_version.in.cpp`` next to your application sources.
This file acts as a template.
During configuration, CMake generates the final source file in the build directory and automatically adds it to the
target.

.. code-block:: cpp

    #include "MyApplication.hpp"

    using namespace el::text::literals;

    auto MyApplication::appVersion() noexcept -> el::Version {
        return el::Version{
                @ERBSLAND_GIT_VERSION_MAJOR@U,
                @ERBSLAND_GIT_VERSION_MINOR@U,
                @ERBSLAND_GIT_VERSION_REVISION@U,
                @ERBSLAND_GIT_VERSION_BUILD@U,
        };
    }

    auto MyApplication::appVersionText() noexcept -> el::StringView {
        return "@ERBSLAND_GIT_VERSION_TEXT@"_el;
    }

Finally, register the template with the target that contains ``MyApplication``.

.. code-block:: cmake

    add_executable(my-app
        src/main.cpp
        src/Application.hpp
        src/Application.cpp
        # ... more sources ...
    )

    erbsland_core_add_git_version(
        TARGET my-app
        PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../"
        TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/src/MyApplication_version.in.cpp"
    )

Direct Application Pattern
==========================

This pattern is useful when ``main()`` creates
:cpp:class:`Application <erbsland::core::Application>` directly and
assigns the main function with
:cpp:func:`Application::setMainFn() <erbsland::core::Application::setMainFn>`.

Create a small ``AppVersion.hpp`` header that declares free functions for accessing the generated version information.

.. code-block:: cpp

    #pragma once

    #include <erbsland/StringView.hpp>
    #include <erbsland/Version.hpp>

    [[nodiscard]] auto appVersion() noexcept -> el::Version;
    [[nodiscard]] auto appVersionText() noexcept -> el::StringView;

Create the matching ``AppVersion.in.cpp`` template:

.. code-block:: cpp

    #include "AppVersion.hpp"

    auto appVersion() noexcept -> el::Version {
        return el::Version{
                @ERBSLAND_GIT_VERSION_MAJOR@U,
                @ERBSLAND_GIT_VERSION_MINOR@U,
                @ERBSLAND_GIT_VERSION_REVISION@U,
                @ERBSLAND_GIT_VERSION_BUILD@U,
        };
    }

    auto appVersionText() noexcept -> el::StringView {
        using namespace el::text::literals;
        return "@ERBSLAND_GIT_VERSION_TEXT@"_el;
    }

After creating the application object, use these functions to populate the application metadata.

.. code-block:: cpp

    #include "AppVersion.hpp"

    #include <erbsland/Application.hpp>

    auto main(int argc, char *argv[]) -> int {
        using namespace el::text::literals;

        el::Application app{argc, argv};
        app.info().setApplicationName("MyApp"_el);
        app.info().setApplicationVersion(appVersion());

        app.setMainFn([]() -> el::ExitCode {
            // do something
            return el::ExitCode::success();
        });

        return app.run();
    }

Register the template with the target:

.. code-block:: cmake

    add_executable(my-app)

    erbsland_core_add_git_version(
        TARGET my-app
        PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../"
        TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/src/AppVersion.in.cpp"
    )

CMake Setup
===========

When Erbsland Core is added as a subdirectory, the helper becomes available immediately after the ``add_subdirectory()``
call.

.. code-block:: cmake

    add_subdirectory(erbsland)

    add_library(my-lib STATIC)

    erbsland_core_add_git_version(
        TARGET my-lib
        PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../"
        TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/src/version.in.cpp"
    )

Installed packages provide the same helper through ``find_package(erbsland-core)``.

By default, the generated source file is written below:

.. code-block:: text

    ${CMAKE_CURRENT_BINARY_DIR}/<target>-git-version/

Specify ``OUTPUT`` only when a target requires a specific filename or location for the generated file.

.. code-block:: cmake

    erbsland_core_add_git_version(
        TARGET my-lib
        PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../"
        TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/src/version.in.cpp"
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/MyLibVersion.cpp"
        VARIABLE_PREFIX MY_LIB_VERSION
    )

With this custom prefix, placeholders such as ``@MY_LIB_VERSION_MAJOR@`` and ``@MY_LIB_VERSION_TEXT@`` become available
in the template.
