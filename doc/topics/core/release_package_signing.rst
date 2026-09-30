..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Release Packages; Signing
    single: Release Packages; Notarization

***************************************
Signing and Notarizing Release Packages
***************************************

A release ZIP can be assembled without credentials, which is useful while developing the packaging rules.
When you distribute the application, signing gives the recipient a way to check who produced its executable files and
whether they have changed.
On macOS, notarization adds an Apple-issued ticket to a signed app.
This page connects those platform procedures to the package tool and shows how to check the extracted result.
For the first package and the general configuration format, see :doc:`release_packages` and
:doc:`release_package_configuration`.

What the Two Checks Mean
========================

Windows code signing uses a publisher certificate and its private key to sign executable files.
The package tool signs the staged executable and each staged non-system DLL, then asks SignTool to verify each
signature.
It requires an RFC 3161 timestamp server and requests SHA-256 for both file and timestamp digests.
The ZIP itself is an archive of those signed files; it is not signed as a separate artifact.

On macOS, the tool signs nested libraries and frameworks first, then signs and verifies the enclosing app.
It enables the hardened runtime and asks for a secure timestamp.
With notarization enabled, it submits a temporary ZIP of the signed app to Apple's notary service, waits for an accepted
result, staples the ticket to the app, and validates the staple before creating the final release ZIP.
Notarization in this tool applies only to an ``.app`` bundle.
Apple describes the current requirements in `Notarizing macOS software before distribution
<https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution>`_.

Sign a Windows Release
======================

Acquire a code-signing certificate and private-key access from your chosen certificate authority.
Providers differ: some put the key in the Windows certificate store or a hardware token; others provide a certificate
file with a key in a cryptographic service provider.
Check the provider's installation steps first, then choose the form that matches the package tool's SignTool options.
You also need the Windows SDK's ``signtool.exe`` and an RFC 3161 timestamp URL from your provider.

For a certificate selectable by its SHA-1 thumbprint, a package configuration can use:

.. code-block:: text

    [main]

    [platform.windows.signing]
    enabled: true
    certificate_sha1: "${env:SIGNING_CERTIFICATE_SHA1}"
    timestamp_server: "https://timestamp.example.invalid"

Supply the real timestamp URL and thumbprint in your release environment.
``certificate_sha1`` is the certificate's identifying thumbprint, not the digest algorithm used to sign files; the tool
requests SHA-256 signatures.
For a certificate file paired with a provider and key identifier, use all three values instead:

.. code-block:: text

    [platform.windows.signing]
    enabled: true
    certificate_file: "certificates/publisher.cer"
    csp: "Provider Name From Your CA"
    key: "${env:SIGNING_KEY_IDENTIFIER}"
    timestamp_server: "https://timestamp.example.invalid"

``certificate_file`` is relative to ``package.elcl`` unless absolute.
The exact ``csp`` and ``key`` values come from your certificate or token provider.
The tool requires either ``certificate_sha1`` or the complete file, CSP, and key combination.
The SignTool options behind these settings are described in `Microsoft's SignTool reference
<https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool>`_.

Find the Right SignTool
-----------------------

By default, the tool finds ``signtool.exe`` under the SDK directory named by ``WindowsSdkDir`` and
``WindowsSDKVersion``, using the current host architecture.
A Visual Studio developer shell normally supplies these variables.
If your signing token's provider software runs only with an x64 SignTool, set the *tool* architecture even when the
packaged executable is arm64:

.. code-block:: text

    [platform.windows.signing]
    enabled: true
    certificate_sha1: "${env:SIGNING_CERTIFICATE_SHA1}"
    timestamp_server: "https://timestamp.example.invalid"
    tool_architecture: "x64"

This changes which SignTool executable performs the signing, not the architecture of ``harbor_client.exe``.
If the SDK environment is unavailable, give an explicit ``tool_path``:

.. code-block:: text

    [platform.windows.signing]
    enabled: true
    certificate_sha1: "${env:SIGNING_CERTIFICATE_SHA1}"
    timestamp_server: "https://timestamp.example.invalid"
    tool_path: "C:/Program Files (x86)/Windows Kits/10/bin/10.0.xxxxx.0/x64/signtool.exe"

Replace the versioned path with one present on your build machine.
A relative ``tool_path`` starts at the configuration file's directory.
If signing fails, first check that this executable exists and that it can access the certificate's private key.
Then check the timestamp endpoint and provider-specific token requirements.

Build and Verify on Windows
---------------------------

With the signing section in place, use the normal release installation:

.. code-block:: powershell

    $env:SIGNING_CERTIFICATE_SHA1 = "YOUR_CERTIFICATE_THUMBPRINT"
    cmake --build build --target harbor_client --config Release
    cmake --install build --config Release --component Package-client
    Expand-Archive packages/client-1.4.0-windows-x64.zip -DestinationPath extracted -Force
    signtool verify /pa /v extracted/client-1.4.0/harbor_client.exe

The tool has already verified each staged executable and DLL before publishing the ZIP.
Verifying a file after extraction checks what your recipient will receive.
Repeat the last command for any DLLs in the archive, and launch the extracted program to catch a missing runtime
dependency that is visible only on a clean machine.

Sign and Notarize a macOS App
=============================

For distribution outside the Mac App Store, obtain a **Developer ID Application** certificate through your Apple
Developer account.
Apple's `Developer ID certificate instructions
<https://developer.apple.com/help/account/certificates/create-developer-id-certificates>`_ explain how the authorized
account holder creates a certificate signing request, downloads the issued ``.cer`` file, and installs it in Keychain
Access.
The matching private key must also be present on the signing Mac; downloading only the certificate onto a different
machine is not enough.
You can inspect the installed identity before configuring the package:

.. code-block:: shell

    security find-identity -v -p codesigning

Copy the intended Developer ID Application identity into an environment variable or set it directly in your local
configuration.
For a signed app without notarization, this is sufficient:

.. code-block:: text

    [main]

    [platform.macos]
    app: true
    bundle_id: "dev.example.harbor"

    [platform.macos.signing]
    enabled: true
    identity: "${env:APPLE_SIGNING_IDENTITY}"

The package tool signs the staged bundle after its libraries have been copied and load paths repaired.
An app made from a non-bundle executable gets the configured ``bundle_id`` in its generated ``Info.plist``.
For an existing ``MACOSX_BUNDLE`` target, set its own bundle metadata in CMake as well.

Prepare Notarization Credentials
--------------------------------

Notarization needs a Developer ID-signed app and credentials usable by Apple's ``notarytool``.
One option is an Apple ID, team ID, and app-specific password saved as a keychain profile.
Create that profile on the signing machine:

.. code-block:: shell

    xcrun notarytool store-credentials harbor-release \
        --apple-id "$APPLE_ID" --team-id "$TEAM_ID"

``notarytool`` prompts for the app-specific password and checks the credentials before storing them.
Apple also documents an App Store Connect API key workflow; use the same profile name in either case.
See `Apple's notarytool credential guide
<https://developer.apple.com/documentation/technotes/tn3147-migrating-to-the-latest-notarization-tool>`_ for the current
credential options.
Then enable notarization for the app package:

.. code-block:: text

    [platform.macos.signing]
    enabled: true
    identity: "${env:APPLE_SIGNING_IDENTITY}"
    notarize: true
    notary_profile: "harbor-release"

The ``notary_profile`` is the keychain profile's name, not its password.
The tool waits for Apple's result, and a rejection stops packaging.
If Apple rejects the submission, inspect its notary log with ``xcrun notarytool log`` using the submission ID reported
by Apple; check every nested binary's signature, the Developer ID identity, and hardened runtime settings.
The package's ``app`` value must remain ``true`` for this workflow.

Verify the Extracted macOS App
------------------------------

After the release install, extract and inspect the same app that will be distributed:

.. code-block:: shell

    cmake --build build --target harbor_client --config Release
    cmake --install build --config Release --component Package-client
    mkdir -p extracted
    ditto -x -k packages/client-1.4.0-macos-arm64.zip extracted
    codesign --verify --deep --strict --verbose=2 extracted/client-1.4.0/harbor_client.app
    xcrun stapler validate extracted/client-1.4.0/harbor_client.app
    spctl --assess --type execute --verbose extracted/client-1.4.0/harbor_client.app

The archive name reflects the actual build architecture; replace ``arm64`` if yours differs.
``stapler validate`` is appropriate after notarization is enabled.
For a signed-only build, run ``codesign --verify`` and then test launching the app.
The tool verifies signing and stapling during packaging, while these commands confirm that extraction preserved the
published bundle.

For settings shared across packages or overridden for one target, return to
:doc:`release_package_configuration`.
The complete CMake call is described in :doc:`release_package_cmake`, and
:doc:`/reference/core/release_packages` lists the supported keys.
