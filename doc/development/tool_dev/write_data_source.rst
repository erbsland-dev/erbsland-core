..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***********************
Write a New Data Source
***********************

This guide adds a reusable input source to the maintenance application and connects it to a command.
Start with :doc:`build_and_test` and read :doc:`data_sources` for the source lifecycle.

Use ``tool/src/tool/psl/PublicSuffixSource.hpp`` and its implementation as a small example.
``tool/src/tool/unicode/UnicodeSource.hpp`` shows a source that prepares several related data structures.

1. Define the Source
====================

Derive a default-constructible class from ``tool::frame::DataSource``.
Place it in the namespace and source directory for its dataset, following the layout described in :doc:`framework`.
Keep construction free of file loading and application-service access; those services are available during callbacks.

Provide a public static ``sourceIdentifier()`` returning :cpp:type:`String <erbsland::text::String>`.
Choose lowercase ASCII letter groups separated by single underscores, such as ``public_suffix``.
The identifier must be unique across sources and commands; ``rendering`` is reserved.
It also names the source's configuration section.

Implement the required ``initialize()`` callback.
The base class already provides defaults for a source without configuration: no rules and a no-op ``configure()``.

2. Define and Read Configuration
================================

For a configurable source, override ``configurationRules()`` to describe settings relative to its section.
Add a matching section to an ``.elcl`` file under ``tool/conf/source``.
The main configuration already includes files in that directory.

Override ``configure(configuration)`` to read the validated values and store the settings needed for loading.
Use ``frame::app().files().checked()`` to check configured project paths.
``PublicSuffixSource::configure()`` demonstrates this for an input file.
The protected ``log()`` stream is available during configuration and initialization.

The framework validates with rule version 1 and applies defaults before calling ``configure()``.
Even if every setting has a default, a source with rules requires a regular section.
If your source needs no configuration, keep the base defaults and do not add a section.
See :ref:`development-tool-framework_reference-configuration-loading-and-includes` for the validation contract.

3. Load and Validate Inputs
===========================

Perform loading in ``initialize()`` using ``frame::app().files()``.
Use ``readText()`` for tolerant UTF-8 or ``readData()`` when exact bytes matter.
Text reads handle an optional BOM and retain the remaining UTF-8 bytes without validating the complete input.
The file service keeps inputs inside the selected project.
See :ref:`development-tool-framework_reference-project-file-access` for its path checks.

Validate the dataset before exposing prepared data.
For a pinned upstream text snapshot, verify its manifest, line-ending-independent hashes, and applicable version headers
before parsing it.
``PublicSuffixSource`` demonstrates these checks; ``UnicodeSource`` delegates dataset verification to ``UcdDataset``.
Keep acquisition and replacement of upstream snapshots separate from initialization.

Retain source locations while parsing so later validation can identify the failing file and line.
Report progress with ``log()`` and attach source context to failures using the
:ref:`diagnostic conventions <development-tool-reference-diagnostics>`.
The application marks the source ready only when ``initialize()`` returns successfully.

4. Expose Prepared Data
=======================

Provide const accessors for the verified text or prepared structures your commands need.
Keep ownership in the source and return immutable references for shared data.
Guard access until preparation has completed, as ``PublicSuffixSource::requireInitialized()`` does.

Keep output-specific work in the command.
For example, the Public Suffix source supplies verified text, while the generator parses rules and encodes its output.
The Unicode source also prepares normalization and mapping data because both generators share those inputs.

5. Build and Register the Source
================================

Add the new header and implementation to ``erbsland-core-tool-lib`` through the dataset directory's ``CMakeLists.txt``.
For a new directory, add it with ``add_subdirectory()`` in ``tool/src/tool/CMakeLists.txt``.
The existing ``tool/src/tool/psl/CMakeLists.txt`` shows how to include a source alongside its command.

Include your source header in ``tool/src/main.cpp`` and register it with ``app.registerSource<YourSource>()`` before
``app.run()``.
Registration constructs an instance owned by the application and records its static identifier.
Commands and sources may be registered in either order.

6. Declare and Use the Dependency
=================================

Override the consuming command's ``usedDataSources()`` and return the required static identifiers.
For example, IDNA generation declares its Unicode source:

.. code-block:: cpp

    auto IdnaTool::usedDataSources() const -> el::StringList {
        return el::StringList{unicode::UnicodeSource::sourceIdentifier()};
    }

List every source the command needs.
The application configures all selected sources before initializing any source.
It then initializes them in declaration order and initializes repeated identifiers only once.
Source ``configure()`` callbacks must not access other sources.
The Public Suffix command lists both ``UnicodeSource`` and ``PublicSuffixSource``.

In the command's ``initialize()`` or ``execute()`` callback, retrieve the prepared instance with
``frame::app().source<YourSource>()`` and read its accessors.
Typed lookup checks readiness; it never starts initialization implicitly.

7. Verify the Source
====================

Add tests under ``tool/test/unittest/src`` and include them in that directory's CMake sources.
Cover valid input, invalid configuration, missing files, malformed data, and integrity or version mismatches relevant to
your format.
Verify that access before successful preparation fails and that a command cannot run after source initialization fails.

Use ``PublicSuffixSourceTest`` or ``UnicodeSourceTest`` as examples for dataset-specific tests.
``DataSourceTest`` already covers shared registration and callback ordering.
Tests using ``frame::app()`` need an active application; ``ProjectFixture`` shows how to provide it.

Follow :doc:`build_and_test` to run the tool tests, then try a consuming command with the Debug executable.
Document the source's settings in :doc:`data_sources` and explain snapshot maintenance in the usage section when needed.
