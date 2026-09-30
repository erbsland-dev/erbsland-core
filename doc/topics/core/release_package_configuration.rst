..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: package.elcl
    single: Release Packages; Configuration

************************************
Configuring Release Package Contents
************************************

A CMake call decides which executables belong to a package.
The optional ``package.elcl`` file decides how that package is named and what else goes inside it.
It can also adapt a shared definition for Windows, macOS, a particular package, or one executable.
You can begin with a few lines and add overrides only when the releases actually differ.
See :doc:`release_packages` for the first CMake call and :doc:`release_package_cmake` for its parameters.

Begin with a Shared Configuration
=================================

By default, the tool reads ``package.elcl`` beside the root ``CMakeLists.txt``.
If you pass ``CONFIG`` to ``erbsland_core_package()``, it reads that file instead.
An absent default file is fine; a file that exists must contain ``[main]``:

.. code-block:: text

    [main]
    target_dir: "%{package:name}-%{version}"
    filename_format: "%{package:name}-%{version}-%{sys:platform}-%{sys:architecture}"

These values happen to be the defaults, so this example changes nothing yet.
It makes the two names visible: ``target_dir`` is the directory inside the ZIP, while ``filename_format`` is the ZIP
basename, without ``.zip``.
The settings in ``[main]`` apply to every package registered by this CMake project.
The package tool validates a present file against its embedded ELCL Validation Rules document before it stages files.
Type errors and misspelled values in override sections are reported with their source location, even when the current
build does not select that override.

Choose a Version Source
=======================

Version from CMake: ``version_source``
--------------------------------------

The default ``version_source`` is ``"cmake"``.
The root project's ``project(... VERSION ...)`` supplies the value:

.. code-block:: cmake

    project(harbor VERSION 1.4.0 LANGUAGES CXX)

.. code-block:: text

    [main]
    version_source: "cmake"

With the default filename format, this produces a name containing ``1.4.0``.
If no CMake project version is available, select a file or Git version source instead.

Version File: ``version_file``
------------------------------

Set ``version_source`` to ``"file"`` and give ``version_file`` a path:

.. code-block:: text

    [main]
    version_source: "file"
    version_file: "VERSION.txt"

The path is relative to the directory containing ``package.elcl`` unless it is absolute.
With no extraction pattern, the first nonempty line that does not begin with ``#`` after trimming becomes the version.
A plain ``1.4.0`` file therefore needs only these two settings.

Extract a Version: ``version_pattern``
--------------------------------------

Some projects keep the version inside a longer line, such as ``release = v1.4.0``.
Add ``version_pattern`` to search that file and capture only the version text:

.. code-block:: text

    [main]
    version_source: "file"
    version_file: "VERSION.txt"
    version_pattern: /v([0-9]+[.][0-9]+[.][0-9]+)/

This is an ELCL regular-expression value, written between ``/`` delimiters.
The parser provides an ``re::RegExPtr`` that the package tool uses directly.
The expression must put the version in capture group one.
For the example line, the package version becomes ``1.4.0``.
If the pattern finds no match, packaging stops instead of guessing a version.

Version from Git
----------------

For a release tied to a Git tag, use:

.. code-block:: text

    [main]
    version_source: "git"

The package tool asks Git for the nearest reachable ``vMAJOR.MINOR.PATCH`` tag.
At the tag, ``v1.4.0`` yields ``1.4.0``.
Later commits append the commit distance and abbreviated hash, such as ``1.4.0-3-a1b2c3d``.
Git must be available to CMake, and the source checkout needs a matching reachable tag.

Shape the Archive
=================

Root Directory: ``target_dir``
------------------------------

``target_dir`` controls the root path a user sees after opening the ZIP.
For a package named ``client`` at version ``1.4.0``, this keeps every file beneath ``client/``:

.. code-block:: text

    [main]
    target_dir: "%{package:name}"

.. code-block:: text

    client-1.4.0-windows-x64.zip
    └── client/
        └── harbor_client.exe

The default is ``%{package:name}-%{version}``.
The expanded path must be relative and cannot contain ``.`` or ``..`` elements.

ZIP Name: ``filename_format``
-----------------------------

``filename_format`` controls the archive basename.
For example, this puts the version first:

.. code-block:: text

    [main]
    filename_format: "%{version}-%{package:name}-%{sys:platform}-%{sys:architecture}"

On Windows x64, package ``client`` at version ``1.4.0`` becomes ``1.4.0-client-windows-x64.zip``.
The default is ``%{package:name}-%{version}-%{sys:platform}-%{sys:architecture}``.
Do not include the ``.zip`` suffix in the format.
Like ``target_dir``, the expanded name must be a relative path without ``.`` or ``..`` elements.

Both formats accept ``%{version}``, ``%{version:major}``, ``%{version:minor}``, ``%{version:patch}``,
``%{version:revision}``, ``%{version:build}``, ``%{sys:platform}``, ``%{sys:architecture}``, ``%{project:name}``, and
``%{package:name}``.
``%{target:name}`` is available for a single-target package, where a target is selected for the package-wide format.
Unknown placeholders stop packaging rather than silently producing an incorrect name.

Find Runtime Libraries: ``dependency_directories``
--------------------------------------------------

The package tool searches beside each built executable by default.
If a non-system DLL or macOS library is built elsewhere, add its directory:

.. code-block:: text

    [main]
    dependency_directories: "build/vendor/lib", "build/plugins"

Relative paths start at the directory containing ``package.elcl``; absolute paths also work.
The tool asks CMake to resolve runtime dependencies using these directories.
An unresolved dependency is an error, so this setting is useful when a library is present but not found through the
normal build location.
Directories from later override sections are added to the earlier list.

macOS App Bundles: ``app``
--------------------------

On macOS, ``app`` defaults to ``true``.
This wraps a non-bundle executable in an ``.app`` with an ``Info.plist``; an existing ``MACOSX_BUNDLE`` target is copied
as a bundle.
For a command-line executable that should remain a plain file in the archive, set the option to ``false``:

.. code-block:: text

    [package.cli.platform.macos]
    app: false

``app: false`` places the executable directly under the archive root and any required libraries beside it.
Notarization in this tool requires an app bundle.

Bundle Identifier: ``bundle_id``
--------------------------------

A generated app should have a stable identifier for distribution.
Set ``bundle_id`` on macOS for a package or individual target:

.. code-block:: text

    [platform.macos]
    bundle_id: "dev.example.harbor"

For a generated app without this setting, the tool uses a local identifier based on the package and target names.
An existing ``MACOSX_BUNDLE`` target keeps its own bundle metadata, which you should configure in CMake.

Add Files and Directories
=========================

Each ``*[<section>.files]*`` entry adds a source to the package.
For two files shared by every package, repeat the section-list header:

.. code-block:: text

    [main]

    *[main.files]*
    path: "LICENSE.txt"
    target: "docs"

    *[main.files]*
    path: "config/defaults.elcl"
    target: "config"

``path`` is required.
A relative source path starts at the configuration file's directory; an absolute source path is also accepted.
``target`` is an optional directory *inside* the archive root.
The source filename is preserved, so the entries above yield:

.. code-block:: text

    client-1.4.0-windows-x64.zip
    └── client-1.4.0/
        ├── harbor_client.exe
        ├── config/defaults.elcl
        └── docs/LICENSE.txt

On macOS, the same additional files are siblings of the app bundle inside the archive root:

.. code-block:: text

    client-1.4.0-macos-arm64.zip
    └── client-1.4.0/
        ├── harbor_client.app/
        │   └── Contents/
        │       ├── MacOS/harbor_client
        │       └── Frameworks/
        ├── config/defaults.elcl
        └── docs/LICENSE.txt

The ``files`` entries add files to the package root, not into ``Contents/Resources`` of a copied app bundle.
If the app needs resources inside its bundle, define them as part of the CMake app target so the bundle already contains
them when packaging begins.

When ``path`` names a directory, the tool copies its files into ``target`` while preserving paths relative to that
source directory.
By default it sees only files directly inside the directory; ``recursive: true`` descends into subdirectories:

.. code-block:: text

    *[main.files]*
    path: "assets"
    target: "share"
    recursive: true
    include_pattern: "*.txt", "**/*.txt"
    exclude_pattern: "**/draft-*"

Here ``assets/readme.txt`` becomes ``share/readme.txt``, and ``assets/help/usage.txt`` becomes ``share/help/usage.txt``.
``include_pattern`` and ``exclude_pattern`` are lists of globs matched against a file's path relative to ``assets``.
``*`` and ``?`` stay within one directory; ``**`` can cross directory boundaries.
You may also use ``include_regex`` and ``exclude_regex`` lists for full-path regular-expression matches:

.. code-block:: text

    *[main.files]*
    path: "assets"
    target: "share"
    recursive: true
    include_regex: `(^|.*/)help[.]txt`
    exclude_regex: `(^|.*/)old/.*`

Include patterns and include regexes form one allowed set; exclude matches then remove files from it.
Patterns apply only to directory entries, not to a single file.
The source may not be a symlink, and directory walking skips symlinks.
An absent source or two entries claiming the same destination stops packaging.
Destination paths must be relative and cannot contain ``.`` or ``..`` elements.

Adapt Settings to a Platform or Package
=======================================

Overrides are applied in this order, with later scalar values replacing earlier ones:

.. code-block:: text

    [main]
    [platform.<system>]
    [platform.<system>.<architecture>]
    [package.<name>]
    [package.<name>.platform.<system>]
    [package.<name>.platform.<system>.<architecture>]
    [target.<target>]
    [target.<target>.platform.<system>]
    [target.<target>.platform.<system>.<architecture>]

``<system>`` is ``windows`` or ``macos``.
The architecture is normalized to names such as ``x64`` and ``arm64``.
Only sections that match the package and current target are used.
File entries and dependency directories are cumulative: later sections add them instead of replacing earlier entries.

For example, every package can include a license, while a Windows arm64 release gets an additional guide:

.. code-block:: text

    [main]
    filename_format: "%{package:name}-%{version}-%{sys:platform}-%{sys:architecture}"

    *[main.files]*
    path: "LICENSE.txt"
    target: "docs"

    [platform.windows]
    target_dir: "windows/%{package:name}-%{version}"

    [platform.windows.arm64]
    filename_format: "%{package:name}-%{version}-windows-arm64"

    *[platform.windows.arm64.files]*
    path: "docs/windows-arm64.txt"
    target: "docs"

The Windows arm64 ZIP contains both documentation files, and its archive root starts with ``windows/``.
On macOS, the platform sections are ignored and the shared defaults remain in effect.

A package section is useful when one CMake project produces separate client and server archives:

.. code-block:: text

    [package.client]
    filename_format: "harbor_client-%{version}-%{sys:platform}-%{sys:architecture}"

    *[package.client.files]*
    path: "config/client.elcl"
    target: "config"

    [package.server.platform.macos]
    app: false

The ``client`` archive receives its configuration file on both systems.
Only the ``server`` package becomes a plain executable on macOS.
The target name can also select settings that follow the executable into every package that includes it:

.. code-block:: text

    [target.harbor_client]
    bundle_id: "dev.example.harbor.client"

    *[target.harbor_client.files]*
    path: "config/client-target.elcl"
    target: "config"

Package and executable target names use regular ELCL name segments: they start with a letter and contain only letters,
digits, and single underscores between words.
Use the same spelling in the CMake call and the configuration section; quoted text-name sections are not accepted.
You can add a platform or architecture subsection under a package or target name in the same way as the shared platform
overrides above.
For a single-target package, the target section can also set ``version_source``, ``target_dir``, and
``filename_format``.
For a package with several targets, define those three shared values in ``[main]``, a platform section, or a package
section; the target sections customize each executable's staging.

Keep Secrets Out of the File
============================

ELCL environment placeholders let a shared file refer to values supplied by a release environment:

.. code-block:: text

    [platform.windows.signing]
    enabled: true
    certificate_sha1: "${env:SIGNING_CERTIFICATE_SHA1}"
    timestamp_server: "https://timestamp.example.invalid"

.. code-block:: powershell

    $env:SIGNING_CERTIFICATE_SHA1 = "YOUR_CERTIFICATE_THUMBPRINT"
    cmake --install build --config Release --component Package-client

The environment value is resolved when the configuration is read during packaging.
This keeps machine-specific credentials out of the checked-in file; it does not replace access control for the
environment, certificate, or key.
See :doc:`release_package_signing` for the required signing values and a complete Windows or macOS setup.
The compact key list is in :doc:`/reference/core/release_packages`.
