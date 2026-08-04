..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*************************************
Signed macOS Protected-Data Unit Test
*************************************

This standalone project tests the real Secure Enclave protected-data provider.
It is intentionally not referenced by the repository's root CMake build because hosted CI workers cannot be expected to
have a suitable Apple development identity or Secure Enclave access.

The test selects ``PlatformOnly`` before validation, so an internal AES fallback cannot make it pass.
It exercises the provider self-test, public protected-block round trips, provider authentication and tamper rejection,
and rejection of an envelope after its native provider and key have been destroyed.

Xcode Automatic Signing
=======================

Use the Xcode generator and pass your Apple development team through the untracked CMake cache::

    cmake -S test/macos/unittest -B cmake-build-macos-unittest -G Xcode \
        -DERBSLAND_MACOS_DEVELOPMENT_TEAM=<your-team-id>

For the first build on a Mac, target the local Mac explicitly and allow Xcode to register it and create or update the
development provisioning profile::

    xcodebuild \
        -project cmake-build-macos-unittest/erbsland-core-macos-unittest.xcodeproj \
        -scheme erbsland-core-macos-unittest \
        -configuration Debug \
        -destination 'platform=macOS,name=My Mac' \
        -allowProvisioningUpdates \
        -allowProvisioningDeviceRegistration \
        build

After the profile exists, regular CMake builds and CTest runs are sufficient::

    cmake --build cmake-build-macos-unittest --config Debug
    ctest --test-dir cmake-build-macos-unittest -C Debug --output-on-failure

Never add a personal development-team identifier to this file or another tracked project file.

Xcode must have the developer account in its Accounts settings, and the account must be allowed to register devices.
If macOS reports that the provisioning profile does not allow this device, repeat the destination-aware ``xcodebuild``
command above.
Alternatively, register the Mac under Certificates, Identifiers & Profiles in the Apple Developer portal, regenerate a
macOS App Development profile, download it, and open the profile once to install it.

Installed Signing Identity
==========================

With Ninja or another command-line generator, CMake selects the first valid identity reported by
``security find-identity -v -p codesigning``.
Select a different installed identity explicitly when necessary::

    cmake -S test/macos/unittest -B cmake-build-macos-unittest -G Ninja \
        -DERBSLAND_MACOS_CODE_SIGN_IDENTITY="Apple Development: Your Name (TEAMID)"
    cmake --build cmake-build-macos-unittest

This path verifies the standalone build and ordinary signature pipeline only.
It deliberately does not claim the restricted application-identifier entitlements because a matching provisioning
profile cannot be produced safely by a generic command-line generator.
Use the Xcode workflow to run the Secure Enclave tests.

When no valid identity is available, the command-line build applies an ad-hoc signature and prints a warning.
A test run without the Xcode-managed profile is expected to reject Secure Enclave key creation.
