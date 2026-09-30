..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: erbsland_core_package
    single: Release Packages; Reference

***************************
Release Packaging Reference
***************************

``erbsland_core_package()`` registers a Windows or macOS ZIP as an install rule in component ``Package-<name>``.
For a practical introduction, start with :doc:`/topics/core/release_packages`.
The detailed guides cover :doc:`CMake integration </topics/core/release_package_cmake>`,
:doc:`configuration and file entries </topics/core/release_package_configuration>`, and
:doc:`signing and notarization </topics/core/release_package_signing>`.

CMake Interface
===============

.. code-block:: cmake

    erbsland_core_package(
        TARGET <executable> | TARGETS <executable>...
        [NAME <package-name>]
        [CONFIG <configuration-file>]
        [OUTPUT_DIRECTORY <directory>])

``TARGET`` and ``TARGETS`` are mutually exclusive and accept existing executable targets.
``NAME`` defaults to ``CMAKE_PROJECT_NAME`` and must be unique in the build.
Package and executable target names must start with a letter and contain only letters, digits, and single underscores
between words, so they can be used as regular ELCL section names.
``CONFIG`` defaults to the optional root ``package.elcl``; an explicit file must exist.
``OUTPUT_DIRECTORY`` defaults to the root ``packages`` directory.
Relative paths are resolved from the root source directory.
The install rule builds the package utility for ``Release`` and ``RelWithDebInfo``, but the target executables must
already be built.
Other configurations create no ZIP.
On other platforms, the call warns and adds no package install rule; application targets and other install rules are
unaffected.

Configuration Keys
==================

A present ELCL file requires ``[main]``.
The package tool validates it against embedded ELCL Validation Rules before resolving package settings.
Scalar settings are overridden in this order: main, platform, platform architecture, package, package platform, package
platform architecture, target, target platform, target platform architecture.
The platform tokens are ``windows`` and ``macos``.
File entries and dependency directories accumulate across matching sections.
See :doc:`/topics/core/release_package_configuration` for the section syntax and examples.

.. list-table:: Values allowed in the main, platform, package, and target sections
    :header-rows: 1

    *   -   Key
        -   Value
    *   -   ``version_source``
        -   ``cmake`` (default), ``file``, or ``git``.
    *   -   ``version_file``, ``version_pattern``
        -   File path and optional ELCL regular expression with version in capture group one.
    *   -   ``target_dir``
        -   Archive root format; default ``%{package:name}-%{version}``.
    *   -   ``filename_format``
        -   ZIP basename format; default ``%{package:name}-%{version}-%{sys:platform}-%{sys:architecture}``.
    *   -   ``dependency_directories``
        -   List of extra runtime-library search directories.
    *   -   ``app``, ``bundle_id``
        -   macOS app-bundle switch (default true) and generated bundle identifier.

The naming formats accept ``%{version}`` and its ``major``, ``minor``, ``patch``, ``revision``, and ``build`` parts;
``%{sys:platform}``, ``%{sys:architecture}``, ``%{project:name}``, ``%{package:name}``, and ``%{target:name}`` for a
selected single target.

A repeated ``*[<section>.files]*`` entry requires ``path`` and accepts ``target``, ``recursive``, ``include_pattern``,
``exclude_pattern``, ``include_regex``, and ``exclude_regex``.
``path`` and ``target`` refer to source and destination respectively.
Relative source and dependency paths start at the configuration file directory.

Signing Keys
============

Signing is disabled by default.
Place these keys in a matching ``[<section>.signing]`` section.
See :doc:`/topics/core/release_package_signing` for certificate setup and verification.

.. list-table:: Signing values
    :header-rows: 1

    *   -   Platform
        -   Keys
    *   -   Both
        -   ``enabled``
    *   -   Windows
        -   ``timestamp_server``; ``certificate_sha1`` or all of ``certificate_file``, ``csp``,
            ``key``; optional ``tool_architecture`` and ``tool_path``.
    *   -   macOS
        -   ``identity``; optional ``notarize`` and ``notary_profile`` when notarization is enabled.

ELCL text values may use ``${env:VARIABLE}`` to read an environment value at packaging time.
