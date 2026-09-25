..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: erbsland_core_package
    single: CMake; Release Packaging

*****************
Release Packaging
*****************

``erbsland_core_package()`` registers one ZIP package on macOS or Windows.
The helper is available both when Erbsland Core is part of the source build and after
``find_package(erbsland-core CONFIG REQUIRED)``.
Each call creates an install rule in component ``Package-<name>``.

CMake Interface
===============

.. code-block:: cmake

    erbsland_core_package(
            TARGET <executable> | TARGETS <executable>...
            [NAME <package-name>]
            [CONFIG <configuration-file>]
            [OUTPUT_DIRECTORY <directory>])

``TARGET`` and ``TARGETS`` are mutually exclusive and accept existing executable targets.
``NAME`` defaults to the root CMake project name and must be unique within the build.
``CONFIG`` defaults to ``<project-root>/package.elcl``; the default file is optional.
An explicit ``CONFIG`` must exist.
``OUTPUT_DIRECTORY`` defaults to ``<project-root>/packages`` and is independent of ``CMAKE_INSTALL_PREFIX``.

The root project must provide ``project(... VERSION ...)`` when no version source is configured.
The output directory and configuration path are resolved relative to the root source directory.
The target executables must be built before installation.
The package tool itself is built during installation for ``Release`` and ``RelWithDebInfo``.
Other configurations emit a warning and create no ZIP.

Configuration File
==================

The optional ELCL file uses ``[main]`` for shared values.
Later sections override earlier values in this order: ``[platform.<system>]``, ``[platform.<system>.<architecture>]``,
``[package.<name>]``, ``[package.<name>.platform.<system>]``, ``[package.<name>.platform.<system>.<architecture>]``, and
the corresponding ``[target.<target>]`` sections.
The system is ``macos`` or ``windows``.
Target names containing a hyphen or other non-name characters require an ELCL text name, for example
``[target."my-app"]``.
For a single-target package, the target section can override the package version, root directory, and ZIP name.
For a multi-target package, those shared values come from the main, platform, architecture, and package sections; target
sections control each target's files, dependency paths, app settings, and signing.

Supported values in these sections are:

.. list-table:: Package configuration values
    :header-rows: 1

    *   -   Key
        -   Meaning
    *   -   ``version_source``
        -   ``cmake`` (default), ``file``, or ``git``.
    *   -   ``version_file``, ``version_pattern``
        -   Text file and optional regular expression with the version in capture group one.
    *   -   ``target_dir``, ``filename_format``
        -   Archive root directory and ZIP basename formats.
    *   -   ``dependency_directories``
        -   List of directories searched for non-system runtime libraries.
    *   -   ``app``, ``bundle_id``
        -   macOS app creation (on by default) and bundle identifier.

Relative paths in these values and in file entries are based on the ELCL file directory.
The Git source uses the nearest reachable ``vMAJOR.MINOR.PATCH`` tag and appends commit distance and hash for later
commits.
The file source reads the first nonempty, non-comment line unless ``version_pattern`` is set.

File entries are repeated ELCL section-list entries named ``*[<section>.files]*``.
Each entry needs ``path`` and may set ``target`` (destination directory), ``recursive``, ``include_pattern``,
``exclude_pattern``, ``include_regex``, and ``exclude_regex``.
Patterns are lists; glob ``*`` stays within a directory and ``**`` crosses directories.
Regular expressions match the whole path relative to the source directory.
Matching entries from each override layer are added in order.
An existing destination is an error.

Format Placeholders
===================

``target_dir`` defaults to ``%{package:name}-%{version}``.
``filename_format`` defaults to ``%{package:name}-%{version}-%{sys:platform}-%{sys:architecture}``.
These formats accept ``%{version}``, ``%{version:major}``, ``%{version:minor}``, ``%{version:patch}``,
``%{version:build}``, ``%{sys:platform}``, ``%{sys:architecture}``, ``%{project:name}``, ``%{package:name}``, and
``%{target:name}`` when a target is selected.
ELCL values also accept ``${env:VARIABLE}`` from the developer environment.

Signing Configuration
=====================

Signing defaults off.
Under a section such as ``[main.signing]``, ``enabled`` enables signing.
On Windows, set ``timestamp_server`` and either ``certificate_sha1`` or ``certificate_file``, ``csp``, and ``key``.
``tool_path`` overrides SignTool discovery through ``WindowsSdkDir`` and ``WindowsSDKVersion``; ``tool_architecture``
selects the SignTool host binary independently of the application architecture.
The package tool signs and verifies the staged executable and DLLs.

On macOS, ``identity`` selects a code-signing identity; notarization requires a Developer ID Application identity.
The tool signs embedded libraries and frameworks from the inside out with a secure timestamp, then signs and verifies
the app.
``notarize`` requires signing and ``notary_profile`` names credentials saved with
``xcrun notarytool store-credentials``.
The tool submits the app, waits for acceptance, staples the ticket, and validates it.
The resulting ZIP is created with ``ditto`` so executable permissions and framework links survive.

See :doc:`/topics/core/release_packages` for a complete build and verification example.
