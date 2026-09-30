..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: erbsland_core_package
    single: Release Packages; CMake Integration

*****************************
Registering Packages in CMake
*****************************

``erbsland_core_package()`` connects an executable target to a release ZIP.
You register the package while configuring the project; CMake produces it later during installation.
This distinction lets you build and test your normal targets before deciding which archives to publish.
Start with :doc:`release_packages` if you have not yet made a first package.

What a Package Call Registers
=============================

Once Erbsland Core has been added to the CMake tree, place the call after the executable target:

.. code-block:: cmake

    add_subdirectory(erbsland/core)
    add_executable(harbor_app src/main.cpp)
    erbsland_core_setup_application(TARGET harbor_app)

    erbsland_core_package(TARGET harbor_app NAME harbor)

On Windows and macOS, the call checks that ``harbor_app`` is an existing executable target and registers the install
component ``Package-harbor``.
It does not create the ZIP at configuration time.
The package tool is added to the build and built when a release install runs; the application itself must be built
before that install.
The function is available through the submodule's ``add_subdirectory()``.
On Linux, the call warns and returns without adding the package utility or an install rule.
The application target and your own Linux install rules still work.

You can use the function more than once.
Give each call a different ``NAME``, and each package receives its own install component and ZIP:

.. code-block:: cmake

    add_executable(harbor_client src/client.cpp)
    add_executable(harbor_server src/server.cpp)

    erbsland_core_package(TARGET harbor_client NAME client)
    erbsland_core_package(TARGET harbor_server NAME server)
    erbsland_core_package(TARGETS harbor_client harbor_server NAME suite)

The first two calls make independent archives.
The ``suite`` call places both executables in one archive.
The executable targets may be reused across packages; the package names must be unique within the build.

Choose the Executables: ``TARGET`` or ``TARGETS``
=================================================

Exactly one of these keywords is required.
``TARGET`` takes one CMake executable target, while ``TARGETS`` takes a list of them:

.. code-block:: cmake

    erbsland_core_package(TARGET harbor_client NAME client)
    erbsland_core_package(TARGETS harbor_client harbor_server NAME suite)

Do not pass filenames or library targets.
Target names must use regular ELCL name segments: a leading letter followed by letters, digits, or single underscores
between words.
For a single-target package, a matching ``[target.<name>]`` configuration can also change its archive name, archive
root, and version.
In a package with several targets, those package-wide values come from the shared and package sections; target sections
still control each executable's files, dependency search paths, macOS app settings, and signing.
See :doc:`release_package_configuration` for examples of both scopes.

Choose the Package Name: ``NAME``
=================================

``NAME`` identifies the package in its component name, configuration sections, and default ZIP filename:

.. code-block:: cmake

    erbsland_core_package(TARGET harbor_client NAME client)

This call creates component ``Package-client`` and, with project version ``1.4.0``, a default archive such as
``client-1.4.0-windows-x64.zip``.
If you omit ``NAME``, the root CMake project name is used.
Use a different explicit name for each call when registering several packages.
Package names follow the same regular ELCL name rule as target names.
If the project name contains a hyphen or another unsupported character, pass an explicit ``NAME``.

Select a Configuration File: ``CONFIG``
=======================================

Without ``CONFIG``, the function looks for ``package.elcl`` in the root source directory.
That default file is optional, so the first package needs no configuration file.
For a different file, pass a path relative to the root source directory or an absolute path:

.. code-block:: cmake

    erbsland_core_package(
        TARGET harbor_client
        NAME client
        CONFIG packaging/client.elcl)

Here ``packaging/client.elcl`` must already exist when CMake configures the project.
Relative source paths *inside* that file start at ``packaging/``, which makes a self-contained packaging directory
possible.
The file format is covered in :doc:`release_package_configuration`.

Choose Where ZIPs Go: ``OUTPUT_DIRECTORY``
==========================================

The default output directory is ``packages`` below the root source directory.
Use ``OUTPUT_DIRECTORY`` when your release process collects artifacts elsewhere:

.. code-block:: cmake

    erbsland_core_package(
        TARGET harbor_client
        NAME client
        OUTPUT_DIRECTORY dist/windows-and-macos)

The relative path above is resolved from the root source directory.
An absolute directory is also accepted.
The tool creates the output directory when it packages the application and replaces an existing ZIP with the same name
after a successful staging run.
``CMAKE_INSTALL_PREFIX`` does not change this destination.

What Happens During Install
===========================

Installing component ``Package-client`` runs the rule registered by the ``client`` call:

.. code-block:: shell

    cmake --build build --target harbor_client --config Release
    cmake --install build --config Release --component Package-client

The release path is:

.. code-block:: text

    CMake install (Release or RelWithDebInfo)
      └── build the Erbsland Core package utility
          └── run the utility with target paths and package settings
              ├── resolve version and configuration overrides
              ├── stage extra files, executables, and runtime libraries
              ├── repair macOS load paths; sign and notarize when enabled
              └── create and publish the ZIP

The utility receives the executable path for the selected build configuration.
If that executable has not been built, installation stops with a build-first error.
Unresolved runtime dependencies, conflicting staged paths, or failed signing also stop the process before the ZIP is
published.
An install without ``--component`` runs all install rules, including every registered package.
If the chosen configuration is neither ``Release`` nor ``RelWithDebInfo``, the rule warns and makes no ZIP.

Next, :doc:`release_package_configuration` explains the optional configuration file and how it customizes each package.
For credentials and release verification, see :doc:`release_package_signing`.
