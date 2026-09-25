..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Packaging; CMake Install
    single: Release Packages

*************************
Creating Release Packages
*************************

Register a package for each application you want to distribute.
The install step stages the executable, its non-system runtime dependencies, and any configured files, then publishes a
ZIP in the project's ``packages`` directory.
Package rules do not depend on other install rules or ``CMAKE_INSTALL_PREFIX``.

Register and Install a Package
==============================

.. code-block:: cmake

    cmake_minimum_required(VERSION 3.28)
    project(example VERSION 1.4.0 LANGUAGES CXX)
    find_package(erbsland-core CONFIG REQUIRED)

    add_executable(example-app main.cpp)
    target_link_libraries(example-app PRIVATE ErbslandDEV::erbsland-core)
    erbsland_core_package(TARGET example-app NAME example)

Build the application first, then run CMake install with a release configuration:

.. code-block:: shell

    cmake --build build --target example-app --config Release
    cmake --install build --config Release --component Package-example

The component invocation creates only ``example``.
Running ``cmake --install build --config Release`` creates every declared package alongside the project's regular
install rules.
``RelWithDebInfo`` also creates packages.
Debug installation reports that packaging was skipped.
A missing application executable produces a build-first error.

Customize the Contents
======================

Create ``package.elcl`` beside the root ``CMakeLists.txt`` when packages need additional files or custom names:

.. code-block:: text

    [main]
    filename_format: "%{package:name}-%{version}-%{sys:platform}-%{sys:architecture}"

    *[main.files]*
    path: "LICENSE.txt"
    target: "docs"

    [platform.macos]
    app: true

    [package.example]
    bundle_id: "dev.example.application"

    *[package.example.files]*
    path: "config/defaults.elcl"
    target: "config"

Each file entry is copied into the package root under its ``target`` directory.
Relative source paths start at the directory containing ``package.elcl``.
Directories can use ``recursive: true`` and include or exclude patterns.
Package and target sections add files after shared sections.
Path collisions and unresolved dependencies stop packaging before the ZIP is published.

For a macOS app, the tool places runtime libraries in ``Contents/Frameworks`` and repairs load paths with CMake's
``BundleUtilities``.
The ZIP preserves app executability and framework links.
On Windows, CMake resolves DLL dependencies and Erbsland Core writes the ZIP.

Sign and Verify a Release
=========================

Signing is an explicit configuration choice.
For Windows, a developer shell with the Windows SDK can select the host SignTool architecture independently of the
target architecture:

.. code-block:: text

    [platform.windows.signing]
    enabled: true
    tool_architecture: "x64"
    certificate_sha1: "${env:SIGNING_CERTIFICATE_SHA1}"
    timestamp_server: "https://timestamp.example.invalid"

For macOS distribution outside the Mac App Store, use a Developer ID Application identity.
Save a notarytool keychain profile before enabling notarization:

.. code-block:: shell

    xcrun notarytool store-credentials release-profile \
        --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$APP_PASSWORD"

.. code-block:: text

    [platform.macos.signing]
    enabled: true
    identity: "${env:APPLE_SIGNING_IDENTITY}"
    notarize: true
    notary_profile: "release-profile"

The installer verifies Windows signatures after SignTool runs.
For a credential-backed Windows release check, extract the ZIP and run:

.. code-block:: powershell

    signtool verify /pa /v .\package-client.exe

On macOS, the installer verifies the signed app and checks that notarization was accepted and stapled.
For a credential-backed macOS release check, extract the ZIP and run:

.. code-block:: shell

    codesign --verify --deep --strict --verbose=2 Example.app
    xcrun stapler validate Example.app
    spctl --assess --type execute --verbose Example.app

Launch the bundled executable after extraction to confirm that its embedded dependencies load.
See :doc:`/reference/core/release_packages` for every configuration key and placeholder.
