*********************************
Development Stages and Versioning
*********************************

Erbsland Core is a young project and is currently in its alpha stage. This page explains how the library will mature,
what users can expect from each development stage, and how releases are versioned.

Alpha Stage
===========

During the alpha stage, our primary focus is building the public API and implementing the core functionality. A first
implementation may be deliberately simple, even when a faster or more robust solution is already conceivable.

This approach allows us to develop the library incrementally. Core components often depend on one another, and
implementing them in their final form too early would either duplicate functionality or rely on assumptions that have
not yet been tested in practice. We therefore begin with a suitable API and a minimal, dependable implementation.

As an API gains internal users, we examine how well it works in real code. If its use feels awkward, requires
unnecessary boilerplate, or leads to recurring workarounds, we revise it. These revisions may include significant and
incompatible changes. Once the API has proved itself, we improve its implementation to make it faster, more robust,
and more efficient.

This incremental process gives both the API and its implementation time to settle. Their quality improves through
practical use rather than speculation.

Alpha releases use version numbers in the ``0.x`` series. Each development iteration increases the minor version
number. The alpha stage ends when Erbsland Core reaches its initial feature-complete state.

Transition from Alpha to Beta
=============================

Once the library is initially feature-complete, we will begin a thorough review of the entire codebase. Every
implementation will be examined to ensure that it meets our quality and security standards.

The review will also cover all unit tests. Here, the main concern is completeness: important behaviour, failure modes,
boundary conditions, and unusual inputs must all be tested. Our goal is to approach complete test coverage and include
as many meaningful edge cases as possible. Achieving this across a library as large as Erbsland Core will be
challenging, but it remains an important target.

After the review is complete and all significant findings have been addressed, the project will enter the beta stage.

Beta Stage
==========

During the beta stage, we will begin using Erbsland Core in commercial projects running in laboratory and test
environments. This is an important step: real applications place far greater demands on a library than isolated tests
and demonstrations. They reveal missing functionality, awkward APIs, unexpected interactions, and implementation
problems that are difficult to anticipate beforehand.

Documentation will become another major focus. By this point, the API should be substantially more stable, allowing us
to invest in comprehensive documentation without repeatedly rewriting or removing large sections.

Beta releases will continue to use version numbers in the ``0.x`` series. The minor version number will increase when
the public API changes.

The beta stage ends when the library has no known release-blocking issues and has demonstrated sufficient stability in
real-world test environments. We will then prepare the first stable release.

First Stable Release
====================

The first stable release will be version ``1.0.0``. It will provide a stable public API, robust implementations, and
comprehensive documentation.

From that point onward, releases will follow semantic versioning:

* A minor version introduces backward-compatible changes or additions to the public API.
* A patch version contains bug fixes or implementation changes that do not alter the documented behaviour of the API.
* A major version introduces incompatible changes to the public API.

