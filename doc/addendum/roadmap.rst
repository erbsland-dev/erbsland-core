*********************************
Development Stages and Versioning
*********************************

Erbsland Core is in beta.
Version ``0.90.0`` is Beta 1, the first beta release.
This page explains the development stages, the remaining work toward a stable release, and the versioning policy.

Alpha Stage
===========

During the alpha stage, our primary focus was building the public API and implementing the core functionality.
Initial implementations allowed us to try APIs in real code before committing to their final design.

Core components depend on one another, so internal applications and demos helped reveal awkward APIs, unnecessary
boilerplate, and recurring workarounds.
We revised those APIs as their usage became clearer.
Alpha releases used version numbers below ``0.90.0``.

Review and Test Coverage
========================

Review of implementation quality, security, and test coverage continues throughout beta.
Beta status does not imply that an independent security audit has been completed.
The cryptography implementation has been reviewed internally; an independent audit remains planned for after the first
mature release.

The review also covers unit tests.
Here, the main concern is completeness: important behaviour, failure modes, boundary conditions, and unusual inputs must
all be tested.
Our goal is to approach complete test coverage and include as many meaningful edge cases as possible.
Achieving this across a library as large as Erbsland Core will be challenging, but it remains an important target.

Beta Stage
==========

During the beta stage, we plan to use Erbsland Core in commercial projects running in laboratory and test environments.
This is an important step: real applications place far greater demands on a library than isolated tests and
demonstrations.
They reveal missing functionality, awkward APIs, unexpected interactions, and implementation problems that are difficult
to anticipate beforehand.

Documentation is another major focus.
As the API becomes more stable, we can invest in comprehensive documentation without repeatedly rewriting or removing
large sections.

Beta releases start with ``0.90.0`` (Beta 1).
The minor version number increases when there are breaking API changes; patch releases contain compatible fixes.
There is no compatibility period for breaking changes during beta.
Pin a tested revision and review the changelog before updating an application.

The beta stage ends when the library has no known release-blocking issues and has demonstrated sufficient stability in
real-world test environments.
We will then prepare the first stable release.

First Stable Release
====================

The first stable release will be version ``1.0.0``.
It will provide a stable public API, robust implementations, and comprehensive documentation.

From that point onward, releases will follow semantic versioning:

* A minor version introduces backward-compatible changes or additions to the public API.
* A patch version contains bug fixes or implementation changes that do not alter the documented behaviour of the API.
* A major version introduces incompatible changes to the public API.

