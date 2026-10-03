..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*******************
Shared Data Sources
*******************

A data source owns the configuration, loading, and validation of reusable inputs.
Commands request sources by identifier and use their prepared data to generate output.
This keeps input settings in one place and lets several commands use the same verified snapshot.

For example, IDNA generation and Public Suffix List generation both use Unicode data.
The shared Unicode source prepares that data once during an invocation, before the selected command runs.
The generators then handle their own output formats, compression, rendering, and file replacement.

Existing Sources
================

Unicode
-------

``tool::unicode::UnicodeSource`` loads a pinned Unicode release from ``tool/data/ucd`` by default.
Its settings are specified in :ref:`development-tool-reference-unicode-configuration`.

The source verifies file hashes and version information before preparing Unicode records, IDNA attributes, NFC
normalization data, and UTS #46 mappings.
Commands access these through ``dataset()``, ``unicodeData()``, ``idnaData()``, ``normalizer()``, and ``mapping()``.
The prepared data follows the selected snapshot rather than Core's compiled Unicode tables.

See :doc:`../tool_reference/idna_generator_reference` for the input files and IDNA derivation, and
:doc:`../tool_reference/public_suffix_generator_reference` for their use in rule preparation.

Public Suffix List
------------------

``tool::psl::PublicSuffixSource`` loads the checked-in Public Suffix List snapshot.
Its configuration and manifest format are specified in :doc:`../tool_reference/public_suffix_generator_reference`.

A sibling ``manifest.elcl`` records the input's line-ending-independent SHA3-256 hash and snapshot version.
The source verifies the hash, reads UTF-8 in tolerant mode with automatic BOM handling, and checks that the version
agrees with the snapshot header.
It exposes the verified text and version through ``text()`` and ``version()``.
The command performs rule parsing and Unicode processing after both of its sources are ready.

Configuration and Ownership
===========================

The application owns each registered source for its lifetime.
A source's static ``sourceIdentifier()`` identifies both its registration and its configuration section.
Sources share the identifier namespace with commands and have no command-line options.

Source configuration belongs in ``tool/conf/source``.
The main configuration includes the ``.elcl`` files in this directory.
A source with configuration rules requires a matching regular section; defaults are applied during validation.
A source without rules must have no matching section and receives ``nullptr`` in ``configure()``.

Initialization
==============

The selected command lists the sources it needs in ``usedDataSources()``.
The application resolves that list, removes duplicate identifiers while preserving order, and validates every required
source section before calling any source callbacks.

For each required source, the application assigns logging, calls ``configure()``, then calls ``initialize()``.
Only successful initialization makes the source available to the command.
All required sources are ready before the command's own ``configure()``, ``initialize()``, and ``execute()`` callbacks.
A source failure prevents these command callbacks from running.

Unused sources are not initialized and their configuration sections are not validated.
The complete configuration and its included files must still be syntactically valid.
Sources do not declare their own dependency graph; commands list all the sources they need.
See :ref:`development-tool-framework_reference-startup-sequence` for the complete startup sequence.

Access Prepared Data
====================

Within a command callback, use typed lookup to retrieve a declared source:

.. code-block:: cpp

    const auto &source = frame::app().source<unicode::UnicodeSource>();
    const auto &data = source.idnaData();

``source<T>()`` checks the identifier, concrete type, and initialization state.
It throws :cpp:class:`LogicFailure <erbsland::err::LogicFailure>` if the requested source is unregistered, has a
different type, or is not ready.
Lookup does not load data.
Sources expose prepared data through immutable references owned by the application.

Extend or Update a Source
=========================

Follow :doc:`write_data_source` to add a new source implementation.
Loading remains offline: acquiring a new snapshot is a separate maintenance operation.
For existing snapshots, follow :doc:`../tool_usage/update_unicode` or :doc:`../tool_usage/update_public_suffix`.
